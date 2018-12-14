#include "jfp.h"

#include "common.h"
#include "fp.h"
#include "remote_monitor.h"
#include "tasks.h"

#define ENABLE_NOTES false
#define NOTE(msg) if (ENABLE_NOTES) printf("%s\n", std::string(msg).c_str())

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

	_assert_rules();
	
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
	NOTE("RESOLVING " + cf_id_.to_string());
	pushed_flag_=false;

	request_requested_dfs(env);

	// Has pushed?

	pushed_flag_=CF::has_pushes(j_);

	if (pushed_flag_) {
		NOTE("PUSHWAITING " + cf_id_.to_string());
		env->df_pusher().open(cf_id_, [this, env](const Id &dfid,
				const ValuePtr &val) {
			ctx_.set_df(dfid, val);
			NOTE("PUSH RECV: " + cf_id_.to_string() + " << " 
				+ dfid.to_string());
			check_exec(env);
		});
	}

	check_exec(env);
}

void JfpExec::request_requested_dfs(const EnvironPtr &env)
{
	auto req_dfs=CF::get_requested_dfs(j_, ctx_);
	
	std::string missing="";
	for (auto df : req_dfs) {
		missing+=" " + df.to_string();
	}

	for (auto dfid : req_dfs) {
		if (requested_.find(dfid)!=requested_.end()) {
			continue;
		} else {
			requested_.insert(dfid);
			NOTE("REQUEST " + cf_id_.to_string()
				+ " <= " + dfid.to_string());
		}
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
}

NodeId JfpExec::get_next_node(const EnvironPtr &env, const Id &id)
{
	assert(id.size()>=2);
	return (id[1]) % env->comm().get_size();
}

void JfpExec::check_exec(const EnvironPtr &env)
{
	request_requested_dfs(env);
	if (CF::is_ready(fp(), j_, ctx_)) {
		exec(env);

		if (pushed_flag_) {
			env->df_pusher().close(cf_id_);
		}
	}
}

void JfpExec::exec(const EnvironPtr &env)
{
	if (j_["type"]=="exec") { exec_exec(env); }
	else if (j_["type"]=="for") { exec_for(env); }
	else if (j_["type"]=="while") { exec_while(env); }
	else if (j_["type"]=="if") { exec_if(env); }
	else {
		ABORT("CF type not implemented: " + j_["type"].dump());
	}
}

void JfpExec::exec_exec(const EnvironPtr &env)
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

void JfpExec::exec_for(const EnvironPtr &env)
{
	assert(j_["type"]=="for");

	if (!CFFor::is_unroll_at_once(j_)) {
		NIMPL // not unroll-at-once strategy?
	}

	int first=(int)(*ctx_.eval(j_["first"]));
	int last=(int)(*ctx_.eval(j_["last"]));
	for (int idx=first; idx<=last; idx++) {
		Context child_ctx=ctx_;
		child_ctx.set_param(j_["var"].get<std::string>(),
			IntValue::create(idx), true);
		spawn_body(env, j_["body"], child_ctx);
	}
	do_afterwork(env);
}

void JfpExec::exec_while(const EnvironPtr &env)
{
	assert(j_["type"]=="while");

	if (!CFFor::is_unroll_at_once(j_)) {
		NIMPL // not unroll-at-once strategy?
	}

	int start=(int)(*ctx_.eval(j_["start"]));
	Context child_ctx=ctx_;
	child_ctx.set_param(j_["var"].get<std::string>(),
		IntValue::create(start), true);
	int cond=(int)(*child_ctx.eval(j_["cond"]));
	if (cond==0) {
		df_computed(env, j_["wout"]["ref"], ValuePtr(new IntValue(start)));

		do_afterwork(env);
	} else {
		spawn_body(env, j_["body"], child_ctx);

		// TODO remove code duplication with spawnbody

		Id item_id=env->create_id("_anon_"+j_["type"].get<std::string>());
		json j1=j_;
		j1["start"]={{"type", "iconst"}, {"value", start+1}};
		JfpExec *item=new JfpExec(fp_id_, item_id, j1.dump(), fact_);
		item->ctx_=ctx_;
		TaskPtr task(item);
		// push context

		NodeId item_node=get_next_node(env, item_id);
		if (item_node==env->comm().get_rank()) {
			env->submit(task);
		} else {
			env->submit(TaskPtr(new Delivery(LocatorPtr(
				new CyclicLocator(item_node)), task)));
		}
	}
}

void JfpExec::exec_if(const EnvironPtr &env)
{
	assert(j_["type"]=="if");

	int cond=(int)(*ctx_.eval(j_["cond"]));
	if (cond==0) {
	} else {
		spawn_body(env, j_["body"], ctx_);
	}
	do_afterwork(env);
}

void JfpExec::exec_struct(const EnvironPtr &env, const json &sub)
{
	Context child_ctx;
	// map sub parameters
	for (auto i=0u; i<sub["args"].size(); i++) {
		auto param=sub["args"][i];
		ValuePtr arg;
		if (param["type"]=="name") {
			assert(j_["args"][i]["type"]=="id");
			arg=ValuePtr(new NameValue(ctx_.eval_ref(
				j_["args"][i]["ref"])));
		} else {
			arg=ctx_.eval(j_["args"][i]);
		}
		child_ctx.set_param(param["id"], ctx_.cast(param["type"], arg));
	}

	spawn_body(env, sub["body"], child_ctx);

	do_afterwork(env);
}

void JfpExec::spawn_body(const EnvironPtr &env, const json &body,
		const Context &base_ctx)
{
	Context ctx; ctx=base_ctx;
	// create local dfs
	std::map<Name, Id> dfs_names;

	for (auto bi : body) {
		if (bi["type"]=="dfs") {
			// Gather & create dfs names
			for (auto name : bi["names"]) {
				Name sname=name.get<std::string>();
				if (dfs_names.find(name)!=dfs_names.end()) {
					fprintf(stderr, "JfpExec::spawn_body: duplicate "
						"df identifier (%s) in sub %s\n",
						sname.c_str(),
						j_["code"].get<std::string>().c_str());
					abort();
				}
				dfs_names[sname]=env->create_id(sname);
			}
		} else {
			// create cfs names
			if (bi.find("id")!=bi.end()) {
				Name name=bi["id"][0].get<std::string>();
				ctx.set_name(name, env->create_id(name));
			}
		}
	}

	for (auto el : dfs_names) {
		ctx.set_name(el.first, el.second);
	}

	for (auto bi : body) {
		if (bi["type"]!="dfs") {
			Id item_id;
			if (bi.find("id") != bi.end()) {
				item_id=ctx.eval_ref(bi["id"]);
			} else {
				item_id=env->create_id("_anon_" 
					+ bi["type"].get<std::string>());
			}
			JfpExec *item=new JfpExec(fp_id_, item_id, bi.dump(), fact_);
			TaskPtr task(item);
			// push context
			init_child_context(item, ctx);

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

	do_afterwork(env);
}

void JfpExec::do_afterwork(const EnvironPtr &env)
{
	NOTE("AFTERWORK: " + cf_id_.to_string());
	// Afterpush
	for (auto push : CF::get_afterpushes(j_, ctx_)) {
		Id dfid=push.first;
		Id cfid=push.second;
		NOTE("PUSHING " + dfid.to_string() + " >> "
			+ cfid.to_string());
		NodeId next_rank=get_next_node(env, cfid);
		if (next_rank==env->comm().get_rank()) {
			env->df_pusher().push(cfid, dfid, ctx_.get_df(dfid));
		} else {
			env->submit(TaskPtr(new Delivery(LocatorPtr(
				new CyclicLocator(next_rank)), TaskPtr(
				new SubmitDfToCf(dfid, ctx_.get_df(dfid), cfid)))));
		}
	}
	for (auto dfid : CF::get_afterdels(j_, ctx_)) {
		TaskPtr task(new DelDf(dfid));;
		NodeId next_rank=get_next_node(env, dfid);
		if (next_rank==env->comm().get_rank()) {
			env->submit(task);
		} else {
			env->submit(TaskPtr(new Delivery(LocatorPtr(
				new CyclicLocator(next_rank)), task)));
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
		int req_count;
		if (CF::is_requested_unlimited(j_, ctx_, id)) {
			req_count=-1;
		} else {
			req_count=CF::get_requests_count(j_, ctx_, id);
		}
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

void JfpExec::init_child_context(JfpExec *child, const Context &ctx)
{
	child->ctx_.pull_names(ctx); // maybe too much, but generally every
	// body item can address any other body item
	child->ctx_.pull_params(ctx); // maybe too much to clone?

	if (child->j_["type"]=="exec") {
		if (child->j_["args"].empty()) {
			return;
		} else {
			for (auto arg : child->j_["args"]) {
				init_child_context_arg(child, arg, ctx);
			}
		}
	} else if (child->j_["type"]=="for" || child->j_["type"]=="while"
			|| child->j_["type"]=="if") {
		child->ctx_=ctx;
	} else {
		fprintf(stderr, "JfpExec::init_child_context_arg: "
			"type not supported,"
			" type=%s, child=%s\n", child->j_["type"].get<std::string>()
				.c_str(), child->j_.dump(2).c_str());
		ABORT("type=" + child->j_["type"].dump());
	}
}

void JfpExec::init_child_context_arg(JfpExec *child,
	const json &arg, const Context &ctx)
{
	if (arg["type"]=="id") {
		Name name=arg["ref"][0].get<std::string>();
		if (!child->ctx_.has_param(name)) {
			child->ctx_.pull_name(name, ctx);
		}

		for (auto i=1u; i<arg["ref"].size(); i++) {
			init_child_context_arg(child, arg["ref"][i], ctx);
		}
	} else if (arg["type"]=="iconst" || arg["type"]=="rconst"
			|| arg["type"]=="sconst") {
		// do nothing
	} else if (arg["type"]=="icast") {
		init_child_context_arg(child, arg["expr"], ctx);
	} else if (arg["type"]=="+" || arg["type"]=="/"
			|| arg["type"]=="*" || arg["type"]=="-") {
		for (auto op : arg["operands"]) {
			init_child_context_arg(child, op, ctx);
		}
	} else {
		fprintf(stderr, "JfpExec::init_child_context_arg: arg type"
			" not supported: %s in %s\n",
			arg["type"].dump().c_str(), arg.dump(2).c_str());
		NIMPL
	}
}

void JfpExec::_assert_rules()
{
#ifdef NDEBUG
	return;
#endif
	if (j_.find("rules")==j_.end()) { return; }
	for (auto rule : j_["rules"]) {
		assert(rule["type"]=="rule");
		if (rule["ruletype"]=="flags") {
			assert(rule.find("flags")!=rule.end());
			for (auto flag : rule["flags"]) {
				if(flag!="has_pushes"
						&& flag!="unroll_at_once"
				) {
					ABORT("Unknown flag: " + flag.dump());
				}
			}
		} else if (rule["ruletype"]=="enum") {
			assert(rule.find("property")!=rule.end());
			assert(rule.find("items")!=rule.end());
			if (rule["property"]!="request"
					&&rule["property"]!="req_unlimited") {
				ABORT("Unknown enum property: " + rule["property"].dump());
			}
		} else if (rule["ruletype"]=="assign") {
			assert(rule.find("property")!=rule.end());
			assert(rule.find("id")!=rule.end());
			assert(rule.find("val")!=rule.end());
			if (rule["property"]!="req_count") {
				ABORT("Unknown assign property: " 
					+ rule["property"].dump());
			}
		} else if (rule["ruletype"]=="map") {
			assert(rule.find("property")!=rule.end());
			assert(rule.find("id")!=rule.end());
			assert(rule.find("expr")!=rule.end());
			if (rule["property"]!="afterpush") {
				ABORT("Unknown map property: " + rule["property"].dump());
			}
		} else if (rule["ruletype"]=="indexed") {
			assert(rule.find("dfs")!=rule.end());
		} else {
			fprintf(stderr, "rule: %s\n", rule.dump(2).c_str());
			ABORT("Unknown rule type: " + rule["ruletype"].dump());
		}
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
