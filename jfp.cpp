// TODO: use locators instead of dfid hash
#include "jfp.h"

#include "common.h"
#include "fp.h"
#include "remote_monitor.h"
#include "tasks.h"

JfpExec::JfpExec(const Id &fp_id, const Id &cf_id, const std::string &jdump,
	Factory &fact)
	: fact_(fact), fp_id_(fp_id), cf_id_(cf_id),
		j_(json::parse(jdump))
{
}

JfpExec::JfpExec(BufferPtr &buf, Factory &fact)
	:fact_(fact)
{
	fp_id_=Id(buf);
	cf_id_=Id(buf);
	j_=json::parse(Buffer::popString(buf));

	ctx_=Context(buf, fact);
}

void JfpExec::run(const EnvironPtr &env)
{
	fp_=&dynamic_cast<const JsonValue*>(env->get(fp_id_).get())->value();
	
	NodeId next_node=get_next_node(env, cf_id_);

	if (next_node!=env->comm().get_rank()) {
		Buffers bufs;
		serialize(bufs);
		env->send(next_node, bufs);
		return;
	}

	resolve_args(env);
}

void JfpExec::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_JfpExec));
	fp_id_.serialize(bufs);
	cf_id_.serialize(bufs);
	bufs.push_back(Buffer::create(j_.dump()));

	ctx_.serialize(bufs);
}


std::string JfpExec::to_string() const
{
	return "JfpExec(" + cf_id_.to_string() + ")";
}

void JfpExec::resolve_args(const EnvironPtr &env)
{
	pushed_flag_=false;

	// Request DFs

	auto req_dfs=CF::get_requested_dfs(j_, ctx_);

	for (auto dfid : req_dfs) {
		NodeId df_node=get_next_node(env, dfid);
		if (df_node==env->comm().get_rank()) {
			env->df_requester().request(dfid, [this, dfid, env](
					const ValuePtr &val){
				ctx_.set_df(dfid, val);
				check_exec(env);
			});
		} else {
			auto rptr=RPtr(env->comm().get_rank(), remote_callback(
					[this, dfid, env](BufferPtr &buf) {
				ValuePtr val=fact_.pop<Value>(buf);
				ctx_.set_df(dfid, val);
				check_exec(env);
			}));
			env->submit(TaskPtr(new Delivery(LocatorPtr(
				new CyclicLocator(df_node)), TaskPtr(new RequestDf(
				dfid, LocatorPtr(new CyclicLocator(env->comm()
				.get_rank())), rptr)))));
		}
	}

	// Has pushed?

	pushed_flag_=CF::has_pushes(j_);

	if (pushed_flag_) {
		env->df_pusher().open(cf_id_, [this, env](const Id &dfid,
				const ValuePtr &val) {
			ctx_.set_df(dfid, val);
			check_exec(env);
		});
	}

	if (!pushed_flag_ && req_dfs.empty()) {
		exec(env);
	}
}

NodeId JfpExec::get_next_node(const EnvironPtr &env, const Id &id)
{
	assert(id.size()>=2);
	return (id[1]) % env->comm().get_size();
}

void JfpExec::check_exec(const EnvironPtr &env)
{
	printf("warning: ensure no new requests are available\n");
	if (CF::is_ready(fp(), j_, ctx_)) {
		exec(env);

		if (pushed_flag_) {
			env->df_pusher().close(cf_id_);
		}
	}
}

void JfpExec::exec(const EnvironPtr &env)
{
	assert(j_["type"]=="exec");

	Name code=j_["code"].get<std::string>();

	if (fp_->find(code.c_str())==fp_->end()) {
		fprintf(stderr, "JfpExec::exec: invalid code name (%s) in %s"
			" in %s\n", code.c_str(), j_.dump(2).c_str(),
			fp_->dump(2).c_str());
		abort();
	}

	auto sub=(*fp_)[code];

	if (sub["type"]=="struct") {
		exec_struct(env, sub);
	} else if (sub["type"]=="extern") {
		exec_extern(env, code);
	} else {
		fprintf(stderr, "JfpExec::exec: invalid sub type (%s) in sub %s\n",
			sub["type"].dump().c_str(), sub.dump(2).c_str());
		abort();
	}
}

void JfpExec::exec_struct(const EnvironPtr &env, const json &sub)
{
	// create local dfs
	std::map<Name, Id> dfs_names;

	for (auto bi : sub["body"]) {
		if (bi["type"]=="dfs") {
			// Gather & create dfs names
			for (auto name : bi["names"]) {
				Name sname=name.get<std::string>();
				if (dfs_names.find(name)!=dfs_names.end()) {
					fprintf(stderr, "JfpExec::exec_struct: duplicate "
						"df identifier (%s) in sub %s\n",
						sname.c_str(),
						j_["code"].get<std::string>().c_str());
					abort();
				}
				dfs_names[sname]=env->create_id(sname);
			}
		} else {
			// create cfs names
			assert(bi.find("id")!=bi.end());
			Name name=bi["id"][0].get<std::string>();
			ctx_.set_name(name, env->create_id(name));
		}
	}

	for (auto el : dfs_names) {
		ctx_.set_name(el.first, el.second);
	}

	for (auto bi : sub["body"]) {
		if (bi["type"]!="dfs") {
			assert(bi.find("id") != bi.end());
			Id item_id=ctx_.eval_ref(bi["id"]);
			JfpExec *item=new JfpExec(fp_id_, item_id, bi.dump(), fact_);
			TaskPtr task(item);
			// push context
			init_child_context(item);

			NodeId item_node=get_next_node(env, item_id);
			if (item_node==env->comm().get_rank()) {
				env->submit(task);
			} else {
				env->submit(TaskPtr(new Delivery(LocatorPtr(
					new CyclicLocator(item_node)), task)));
			}
		}
	}
}

void JfpExec::exec_extern(const EnvironPtr &env, const Name &code)
{
	std::vector<ValuePtr> args;
	auto ext=fp()[code];
	for (auto i=0u; i<ext["args"].size(); i++) {
		bool is_input=fp()[code]["args"][i]["type"]!="name";
		if (is_input) {
			args.push_back(ctx_.eval(j_["args"][i]));
		} else {
			args.push_back(ValuePtr(nullptr));
		}
	}
	// Algorithm: eval args & call sub; return args save as dfs.
	env->exec_extern(ext["code"].get<std::string>(), args);
	for (auto i=0u; i<ext["args"].size(); i++) {
		bool is_input=fp()[code]["args"][i]["type"]!="name";
		if (!is_input) {
			if (args[i]) {
				df_computed(env, j_["args"][i]["ref"], args[i]);
			} else {
				fprintf(stderr, "extern %s: output parameter %d not set\n",
					code.c_str(), (int)i);
				abort();
			}
		}
	}

	// Afterpush
	for (auto push : CF::get_afterpushes(j_, ctx_)) {
		Id dfid=push.first;
		Id cfid=push.second;
		NodeId next_rank=get_next_node(env, cfid);
		if (next_rank==env->comm().get_rank()) {
			env->df_pusher().push(cfid, dfid, ctx_.get_df(dfid));
		} else {
			env->submit(TaskPtr(new Delivery(LocatorPtr(
				new CyclicLocator(next_rank)), TaskPtr(
				new SubmitDfToCf(dfid, ctx_.get_df(dfid), cfid)))));
		}
	}
}

void JfpExec::df_computed(const EnvironPtr &env, const json &ref,
	const ValuePtr &val)
{
	Id id=ctx_.eval_ref(ref);

	ctx_.set_df(id, val);

	// store if requestable
	if (CF::is_df_requested(j_, ctx_, id)) {
		NodeId store_node=get_next_node(env, id);
		auto req_count=CF::get_requests_count(j_, ctx_, id);
		if (store_node==env->comm().get_rank()) {
			std::shared_ptr<size_t> counter(new size_t(req_count));
			std::shared_ptr<std::mutex> m(new std::mutex());
			env->df_requester().put(id, val, [counter, env, id, m](){
				std::lock_guard<std::mutex> lk(*m);
				assert (*counter>0);
				(*counter)--;

				if (*counter==0) {
					env->df_requester().del(id);
				}
			});
		} else {
			env->submit(TaskPtr(new Delivery(LocatorPtr(
				new CyclicLocator(store_node)), TaskPtr(
				new StoreDf(id, val, nullptr, req_count)))));
		}
	}
}

void JfpExec::init_child_context(JfpExec *child)
{
	child->ctx_.pull_names(ctx_);

	if (child->j_["type"]=="exec") {
		if (child->j_["args"].empty()) {
			return;
		} else {
			for (auto arg : child->j_["args"]) {
				init_child_context_arg(child, arg);
			}
		}
	} else {
		fprintf(stderr, "JfpExec::init_child_context_arg: "
			"type not supported,"
			" type=%s, child=%s\n", child->j_["type"].get<std::string>()
				.c_str(), child->j_.dump(2).c_str());
		abort();
	}
}

void JfpExec::init_child_context_arg(JfpExec *child,
	const json &arg)
{
	if (arg["type"]=="id") {
		Name name=arg["ref"][0].get<std::string>();
		child->ctx_.pull_name(name, ctx_);

		for (auto i=1u; i<arg["ref"].size(); i++) {
			init_child_context_arg(child, arg["ref"][i]);
		}
	} else if (arg["type"]=="iconst") {
		// do nothing
	} else {
		fprintf(stderr, "JfpExec::init_child_context_arg: %s\n",
			arg.dump(2).c_str());
		NIMPL
	}
}

JfpReg::JfpReg(BufferPtr &buf, Factory &)
{
	fp_id_=Id(buf);
	auto s=Buffer::popString(buf);
	j_=json::parse(s);
	rptr_=RPtr(buf);
}

void JfpReg::run(const EnvironPtr &env)
{
	auto ret=env->set(fp_id_, ValuePtr(new JsonValue(j_)));
	assert(!ret);

	env->submit(TaskPtr(new MonitorSignal(rptr_, {})));
}
