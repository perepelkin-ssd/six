#include "jfp.h"

#include "common.h"
#include "fp.h"
#include "remote_monitor.h"
#include "tasks.h"


JfpExec::JfpExec(const Id &fp_id, const Id &cf_id, const std::string &jdump,
	const LocatorPtr &loc, Factory &fact)
	: fact_(fact), fp_id_(fp_id), cf_id_(cf_id),
		j_(json::parse(jdump)), loc_(loc)
{
}

JfpExec::JfpExec(BufferPtr &buf, Factory &fact)
	:fact_(fact)
{
	fp_id_=Id(buf);
	cf_id_=Id(buf);
	j_=json::parse(Buffer::popString(buf));
	loc_=fact.pop<Locator>(buf);
	size_t count=Buffer::pop<size_t>(buf);
	for (auto i=0u; i<count; i++) {
		auto prefix=Id(buf);
		auto j=json::parse(Buffer::popString(buf));
		assert(locators_.find(prefix)==locators_.end());
		locators_[prefix]=j;
	}

	ctx_=Context(buf, fact);
}

void JfpExec::run(const EnvironPtr &env)
{
	fp_=&dynamic_cast<const JsonValue*>(env->get(fp_id_).get())->value();

	_assert_rules();
	
	NodeId next_node=loc_->get_next_node(env->comm());

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
	loc_->serialize(bufs);
	bufs.push_back(Buffer::create<size_t>(locators_.size()));
	for (auto it : locators_) {
		it.first.serialize(bufs);
		bufs.push_back(Buffer::create(it.second.dump()));
	}

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
	
	for (auto dfid : req_dfs) {
		if (requested_.find(dfid)!=requested_.end()) {
			continue;
		} else {
			requested_.insert(dfid);
			NOTE("REQUEST " + cf_id_.to_string()
				+ " <= " + dfid.to_string());
		}
		NodeId df_node=glocate_next_node(dfid, env, ctx_);
		if (df_node==env->comm().get_rank()) {
			NOTE("REQUEST LOCAL " + cf_id_.to_string()
				+ " <= " + dfid.to_string());
			env->df_requester().request(dfid, [this, dfid, env](
					const ValuePtr &val){
				ctx_.set_df(dfid, val);
				NOTE("REQUEST FULFILLED " + cf_id_.to_string() +
					" <= " + dfid.to_string());
				check_exec(env);
			});
		} else {
			NOTE("REQUEST REMOTE " + cf_id_.to_string()
				+ " <= " + dfid.to_string());
			auto rptr=RPtr(env->comm().get_rank(), remote_callback(
					[this, dfid, env](BufferPtr &buf) {
				NOTE("REQUEST REMOTE FF START");
				ValuePtr val=fact_.pop<Value>(buf);
				ctx_.set_df(dfid, val);
				NOTE("REQUEST REMOTE FF " + cf_id_.to_string() +
				" <= " + dfid.to_string());

				check_exec(env);
				NOTE("REQUEST REMOTE FF STOP");
			}));
			env->submit(TaskPtr(new Delivery(LocatorPtr(
				new CyclicLocator(df_node)), TaskPtr(new RequestDf(
				dfid, LocatorPtr(new CyclicLocator(env->comm()
				.get_rank())), rptr)))));
		}
	}
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
	NOTE("JfpExec::exec " + to_string());
	if (j_["type"]=="exec") { exec_exec(env); }
	else if (j_["type"]=="for") { exec_for(env); }
	else if (j_["type"]=="while") { exec_while(env); }
	else if (j_["type"]=="if") { exec_if(env); }
	else {
		ABORT("CF type not implemented: " + j_["type"].dump());
	}
	NOTE("JfpExec::exec done " + to_string());
}

void JfpExec::exec_exec(const EnvironPtr &env)
{
	NOTE("JfpExec::exec_exec " + to_string());
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
		NOTE("JfpExec::exec_exec exec_extern>> " + to_string());
		exec_extern(env, code);
		NOTE("JfpExec::exec_exec exec_extern<< " + to_string());
	} else {
		fprintf(stderr, "JfpExec::exec: invalid sub type (%s) in sub %s\n",
			sub["type"].dump().c_str(), sub.dump(2).c_str());
		abort();
	}
	NOTE("JfpExec::exec_exec done " + to_string());
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
		spawn_body(env, j_, child_ctx);
	}
	do_afterwork(env, ctx_);
}

void JfpExec::exec_while(const EnvironPtr &env)
{
	assert(j_["type"]=="while");

	if (!CFFor::is_unroll_at_once(j_)) {
		printf("%s\n", j_.dump(2).c_str());
		ABORT("Not an unroll-at-once strategy?");
	}

	int start=(int)(*ctx_.eval(j_["start"]));
	Context child_ctx=ctx_;
	child_ctx.set_param(j_["var"].get<std::string>(),
		IntValue::create(start), true);
	int cond=(int)(*child_ctx.eval(j_["cond"]));
	if (cond==0) {
		df_computed(env, j_["wout"]["ref"], ValuePtr(new IntValue(start)));

		do_afterwork(env, child_ctx);
	} else {
		spawn_body(env, j_, child_ctx);

		// TODO remove code duplication with spawnbody

		Id item_id=env->create_id("_anon_"+j_["type"].get<std::string>());
		json j1=j_;
		j1["start"]={{"type", "iconst"}, {"value", start+1}};
		JfpExec *item=new JfpExec(fp_id_, item_id, j1.dump(), loc_, fact_);
		item->ctx_=ctx_;
		item->loc_=CF::get_locator(item->j_, loc_, ctx_);
		if (!item->loc_) {
			ABORT("While item does not have local locator on respawn? o_O");
		}
		item->locators_=locators_;
		TaskPtr task(item);
		env->submit(task);
	}
}

void JfpExec::exec_if(const EnvironPtr &env)
{
	assert(j_["type"]=="if");

	int cond=(int)(*ctx_.eval(j_["cond"]));
	if (cond==0) {
	} else {
		spawn_body(env, j_, ctx_);
	}
	do_afterwork(env, ctx_);
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

	spawn_body(env, sub, child_ctx);

	do_afterwork(env, ctx_);
}

void JfpExec::spawn_body(const EnvironPtr &env, const json &item,
		const Context &base_ctx)
{
	auto body=item["body"];
	NOTE("spawn_body " + cf_id_.to_string() + " =>* " 
		+ std::to_string(body.size()));
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
			} else if (bi["type"]!="while"
				&& bi["type"]!="if"
				&& bi["type"]!="for") {
				WARN(bi.dump(2)+ " <- no id in cf of type " +
					bi["type"].dump());
			}
		}
	}

	for (auto el : dfs_names) {
		ctx.set_name(el.first, el.second);
	}

	// set global locators
	for (auto it : CF::get_global_locators(item, ctx)) {
		if (locators_.find(it.first)!=locators_.end()) {
			ABORT("Locator redefinition for " + it.first.to_string());
		}
		locators_[it.first]=it.second;
	}

	// set local locators
	for (auto bi : body) {
		if (bi["type"]!="dfs") {
			// create cfs names
			if (bi.find("id")!=bi.end()) {
				Id prefix=ctx.eval_ref(json({bi["id"][0]}));
				if(locators_.find(prefix)==locators_.end()) {
					locators_[prefix]=CF::get_loc_spec(bi);
				}
			}
		}
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
			LocatorPtr loc=CF::get_locator(bi, loc_, ctx);
			if (!loc) {
				loc=get_global_locator(item_id, env, ctx);
			}
			if (!loc) {
				ABORT("Failed to deduce locator");
				// Or try to submit a trivial one?
			}
			JfpExec *item=new JfpExec(fp_id_, item_id, bi.dump(), loc,
				fact_);
			TaskPtr task(item);
			// push context
			init_child_context(item, ctx);
			env->submit(task);
		}
	}
}

void JfpExec::exec_extern(const EnvironPtr &env, const Name &code)
{
	std::vector<CodeLib::Argument> args;
	auto ext=fp()[code];
	for (auto i=0u; i<ext["args"].size(); i++) {
		auto param=fp()[code]["args"][i];
		auto arg=j_["args"][i];
		if (param["type"]=="name") {
			args.push_back(std::make_tuple(
				Reference,
				ValuePtr(nullptr),
				ctx_.eval_ref(arg["ref"])
			));
		} else if (param["type"]=="value") {
			args.push_back(std::make_tuple(
				Custom,
				ctx_.cast("value", ctx_.eval(arg)),
				ctx_.eval_ref(arg["ref"])
			));
		} else {
			auto val=ctx_.cast(param["type"], ctx_.eval(arg));
			args.push_back(std::make_tuple(
				val->type(),
				val, Id()));
		}
	}
	// Algorithm: eval args & call sub; return args save as dfs.
	NOTE("EXEC_EXTERN " + cf_id_.to_string());
	env->exec_extern(ext["code"].get<std::string>(), args);
	NOTE("EXEC_EXTERN done" + cf_id_.to_string());

	for (auto i=0u; i<ext["args"].size(); i++) {
		auto param=fp()[code]["args"][i];
		auto arg=j_["args"][i];
		if (param["type"]=="value") {
			// check if grabbed:
			if (!std::get<1>(args[i])) {
				ABORT("Input was grabbed, gotta do something here: "
					+ std::get<2>(args[i]).to_string());
			}
		} else if (param["type"]=="name") {
			if (std::get<1>(args[i])) {
				df_computed(env, arg["ref"], std::get<1>(args[i]));
			} else {
				fprintf(stderr, "extern %s: output parameter %d not set\n",
					code.c_str(), (int)i);
				abort();
			}
		}
	}

	do_afterwork(env, ctx_);
	NOTE("afterwork done1 " + to_string() + " of " + std::to_string(args.size()));
	for (auto arg : args) {
		NOTE(std::string("ARG: ") + std::to_string(std::get<0>(arg))
			+ " " + std::get<1>(arg)->to_string()
			+ " " + std::get<2>(arg).to_string());
	}
	while (!args.empty()) {
		auto arg=args.back();
		NOTE(std::string("ARG: ") + std::to_string(std::get<0>(arg))
			+ " " + std::get<1>(arg)->to_string()
			+ " " + std::get<2>(arg).to_string());
		args.pop_back();
		NOTE("deleted");
	}
	NOTE("afterwork done2 " + to_string());
}

void JfpExec::do_afterwork(const EnvironPtr &env, const Context &ctx)
{
	NOTE("AFTERWORK: " + cf_id_.to_string());
	// Afterpush
	for (auto push : CF::get_afterpushes(j_, ctx)) {
		Id dfid=push.first;
		Id cfid=push.second;
		NOTE("PUSHING " + dfid.to_string() + " >> "
			+ cfid.to_string());
		NodeId next_rank=glocate_next_node(cfid, env, ctx_);
		if (next_rank==env->comm().get_rank()) {
			env->df_pusher().push(cfid, dfid, ctx_.get_df(dfid));
		} else {
			env->submit(TaskPtr(new Delivery(LocatorPtr(
				new CyclicLocator(next_rank)), TaskPtr(
				new SubmitDfToCf(dfid, ctx_.get_df(dfid), cfid)))));
		}
	}
	for (auto dfid : CF::get_afterdels(j_, ctx)) {
		TaskPtr task(new DelDf(dfid));;
		NodeId next_rank=glocate_next_node(dfid, env, ctx_);
		if (next_rank==env->comm().get_rank()) {
			env->submit(task);
		} else {
			env->submit(TaskPtr(new Delivery(LocatorPtr(
				new CyclicLocator(next_rank)), task)));
		}
	}
	NOTE("AFTERWORK DONE: " + cf_id_.to_string());
}

void JfpExec::df_computed(const EnvironPtr &env, const json &ref,
	const ValuePtr &val)
{
	Id id=ctx_.eval_ref(ref);
	NOTE("DF COMPUTED: " + id.to_string() + ": " + val->to_string());

	ctx_.set_df(id, val);

	// store if requestable
	if (CF::is_df_requested(j_, ctx_, id)) {
		NodeId store_node=glocate_next_node(id, env, ctx_);
		int req_count;
		if (CF::is_requested_unlimited(j_, ctx_, id)) {
			req_count=-1;
		} else {
			req_count=CF::get_requests_count(j_, ctx_, id);
		}
		if (store_node==env->comm().get_rank()) {
			NOTE("REQ_STORE LOCALLY " + id.to_string());
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
			NOTE("REQ_STORE REMOTELY " + id.to_string() + " at "
				+ std::to_string(store_node));
			env->submit(TaskPtr(new Delivery(LocatorPtr(
				new CyclicLocator(store_node)), TaskPtr(
				new StoreDf(id, val, nullptr, req_count)))));
		}
	}
}

void JfpExec::init_child_context(JfpExec *child, const Context &ctx)
{
	// TODO optimize here:
	child->ctx_.pull_names(ctx); // maybe too much, but generally every
	// body item can address any other body item
	child->ctx_.pull_params(ctx); // maybe too much to clone?

	// maybe too much? only import adressed names
	child->locators_=locators_;


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
			|| arg["type"]=="*" || arg["type"]=="-"
			|| arg["type"]=="%"
	) {
		for (auto op : arg["operands"]) {
			init_child_context_arg(child, op, ctx);
		}
	} else {
		fprintf(stderr, "JfpExec::init_child_context_arg: arg type"
			" not supported: %s in %s\n",
			arg["type"].dump().c_str(), arg.dump(2).c_str());
		ABORT("Type not supported: " + arg["type"].dump());
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
					&&rule["property"]!="delete"
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
		} else if (rule["ruletype"]=="expr") {
			assert(rule.find("property")!=rule.end());
			assert(rule.find("expr")!=rule.end());
			if (rule["property"]!="locator_cyclic") {
				ABORT("Unknown expr property: " + rule["property"].dump());
			}
		} else if (rule["ruletype"]=="indexed") {
			assert(rule.find("dfs")!=rule.end());
		} else {
			fprintf(stderr, "rule: %s\n", rule.dump(2).c_str());
			ABORT("Unknown rule type: " + rule["ruletype"].dump());
		}
	}
}

LocatorPtr JfpExec::get_global_locator(const Id &id, const EnvironPtr &env,
	const Context &ctx)
{
	Id prefix=id;
	while (prefix.size()>=2) {
		if (locators_.find(prefix)!=locators_.end()) {
			return CF::get_global_locator(locators_[prefix],
				std::vector<int>(id.begin()+prefix.size(), id.end()), ctx);
		}
		prefix.pop_back();
	}
	ABORT("No locator found for: " + id.to_string());
}

NodeId JfpExec::glocate_next_node(const Id &dfid, const EnvironPtr &env,
	const Context &ctx)
{
	auto gloc=get_global_locator(dfid, env, ctx);
	auto res=gloc->get_next_node(env->comm());
	return res;
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
