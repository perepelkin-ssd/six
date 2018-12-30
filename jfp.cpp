#include "jfp.h"

#include "common.h"
#include "fp_util.h"
#include "remote_monitor.h"
#include "tasks.h"

extern "C" {
#include "bi.h"
#include "body.h"
#include "expr.h"
#include "ref.h"
#include "rules.h"
#include "sub.h"
}

JfpExec::JfpExec(const Id &fp_id, const Id &cf_id, WORD cf_ofs,
	const LocatorPtr &loc, Factory &fact)
	: fact_(fact), fp_id_(fp_id), cf_id_(cf_id),
		cf_ofs_(cf_ofs), loc_(loc), wstart_is_overriden_(false)
{
}

JfpExec::JfpExec(BufferPtr &buf, Factory &fact)
	:fact_(fact)
{
	fp_id_=Id(buf);
	cf_id_=Id(buf);

	cf_ofs_=Buffer::pop<WORD>(buf);

	loc_=fact.pop<Locator>(buf);
	size_t count=Buffer::pop<size_t>(buf);
	for (auto i=0u; i<count; i++) {
		auto prefix=Id(buf);
		assert(locators_.find(prefix)==locators_.end());
		locators_[prefix]=Buffer::pop<WORD>(buf);
	}

	ctx_=Context(buf, fact);

	wstart_is_overriden_=Buffer::pop<bool>(buf);
	wstart_override_=Buffer::pop<int>(buf);
}
void JfpExec::run(const EnvironPtr &env)
{
	fp_=dynamic_cast<const CustomValue*>(env->get(fp_id_).get())
		->get_data();
	cf_=OFS(fp_, cf_ofs_, const void);

	//_assert_rules();
	
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

	bufs.push_back(Buffer::create<WORD>(cf_ofs_));

	loc_->serialize(bufs);
	bufs.push_back(Buffer::create<size_t>(locators_.size()));
	for (auto it : locators_) {
		it.first.serialize(bufs);
		bufs.push_back(Buffer::create<WORD>(it.second));
	}

	ctx_.serialize(bufs);

	bufs.push_back(Buffer::create<bool>(wstart_is_overriden_));
	bufs.push_back(Buffer::create<int>(wstart_override_));
}

std::string JfpExec::to_string() const
{
	return "JfpExec(" + cf_id_.to_string() + ")";
}

void JfpExec::exec_main(const EnvironPtr &env, const Id &fp_id,
	Factory &fact, const std::string main_arg)
{
	const void *fp=dynamic_cast<const CustomValue*>(env->get(fp_id).get())
		->get_data();
	const void *main_sub=fp_sub_by_name(fp, "main");
	const void *main_body=sub_body(main_sub);
	const void *main_rules=sub_rules(main_sub);
	Context ctx;

	if(sub_params_count(main_sub)!=0) {
		assert(sub_params_count(main_sub)==1);
		assert(sub_partype(main_sub, 0)==PARAM_STRING);
		ctx.set_param(sub_parname(main_sub, 0), ValuePtr(new StringValue(
			main_arg)));
	}

	spawn_body(env, main_body, main_rules, ctx,
		LocatorPtr(new CyclicLocator(env->comm().get_rank())), {}, fp,
		fp_id, fact);
}

void JfpExec::resolve_args(const EnvironPtr &env)
{
	{
		std::lock_guard<std::mutex> lk(m_);
		executed_flag_=false;

		NOTE("RESOLVING " + cf_id_.to_string());
		pushed_flag_=false;

		request_requested_dfs(env);

		// Has pushed?

		pushed_flag_=CF::has_pushes(cf_);

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
	}

	check_exec(env);
}

void JfpExec::request_requested_dfs(const EnvironPtr &env)
{
	auto req_dfs=CF::get_requested_dfs(cf_, ctx_);
	
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
	std::unique_lock<std::mutex> lk(m_);
	if (executed_flag_) { return; }

	request_requested_dfs(env);
	for (auto dfid : requested_) {
		if (!ctx_.has_df(dfid)) {
			return;
		}
	}
	bool ready_flag=false;
	if (wstart_is_overriden_) {
		assert(bi_type(cf_)==BI_WHILE);
		Context ctx1=ctx_;
		ctx1.set_param(bi_var(cf_),
			ValuePtr(new IntValue(wstart_override_)));
		ready_flag=ctx1.can_eval(bi_cond(cf_));
		
	} else {
		ready_flag=CF::is_ready(fp_, cf_, ctx_);
	}

	if (ready_flag) {
		executed_flag_=true;
		lk.unlock();
		exec(env);
		lk.lock();

		if (pushed_flag_) {
			env->df_pusher().close(cf_id_);
		}
	}
}

void JfpExec::exec(const EnvironPtr &env)
{
	NOTE("JfpExec::exec " + to_string());
	switch (bi_type(cf_)) {
		case BI_EXEC: exec_exec(env); break;
		case BI_IF: exec_if(env); break;
		case BI_FOR: exec_for(env); break;
		case BI_WHILE: exec_while(env); break;
			
		default:
			ABORT("CF type not implemented: " 
				+ std::to_string(bi_type(cf_)));
	}
	NOTE("JfpExec::exec done " + to_string());
}

void JfpExec::exec_exec(const EnvironPtr &env)
{
	NOTE("JfpExec::exec_exec " + to_string());
	assert(bi_type(cf_)==BI_EXEC);

	auto sub_idx=bi_sub_idx(cf_);
	auto sub=fp_sub(fp_, sub_idx);

	switch (sub_type(sub)) {
		case SUB_STRUCT:
			exec_struct(env, sub);
			break;
		case SUB_EXTERN:
			NOTE(std::to_string(env->comm().get_rank())
				+": extern " + to_string());
			exec_extern(env, sub);
			NOTE("JfpExec::exec_exec exec_extern<< " + to_string());
			break;
		default:
			ABORT("Illegal sub type: " + std::to_string(sub_type(sub)));
	}
	NOTE("JfpExec::exec_exec done " + to_string());
}

void JfpExec::exec_for(const EnvironPtr &env)
{
	assert(bi_type(cf_)==BI_FOR);

	if (!CFFor::is_unroll_at_once(cf_)) {
		ABORT("not unroll-at-once strategy?");
	}

	int first=(int)(*ctx_.eval(bi_first(cf_)));
	int last=(int)(*ctx_.eval(bi_last(cf_)));
	for (int idx=first; idx<=last; idx++) {
		Context child_ctx=ctx_;
		child_ctx.set_param(bi_var(cf_),
			IntValue::create(idx), true);
		spawn_body(env, bi_body(cf_), bi_rules(cf_), child_ctx, loc_,
			locators_, fp_, fp_id_, fact_);
	}
	do_afterwork(env, ctx_);
}

void JfpExec::exec_while(const EnvironPtr &env)
{
	assert(bi_type(cf_)==BI_WHILE);

	if (!CFFor::is_unroll_at_once(cf_)) {
		ABORT("Not an unroll-at-once strategy?");
	}

	int start=wstart_is_overriden_? wstart_override_
		: (int)(*ctx_.eval(bi_start(cf_)));
	Context child_ctx=ctx_;
	child_ctx.set_param(bi_var(cf_),
		IntValue::create(start), true);
	int cond=(int)(*child_ctx.eval(bi_cond(cf_)));
	if (cond==0) {
		df_computed(env, ctx_.eval_ref(bi_wout(cf_)),
			ValuePtr(new IntValue(start)));

		do_afterwork(env, child_ctx);
	} else {
		spawn_body(env, bi_body(cf_), bi_rules(cf_), child_ctx, loc_,
			locators_, fp_, fp_id_, fact_);

		Id item_id=env->create_id("_anon_while");
		JfpExec *item=new JfpExec(fp_id_, item_id, cf_ofs_, loc_, fact_);
		item->ctx_=ctx_;
		item->wstart_is_overriden_=true;
		item->wstart_override_=start+1;
		item->loc_=CF::get_locator(cf_, loc_, ctx_);
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
	assert(bi_type(cf_)==BI_IF);

	int cond=(int)(*ctx_.eval(bi_cond(cf_)));
	if (cond!=0) {
		spawn_body(env, bi_body(cf_), bi_rules(cf_), ctx_, loc_,
			locators_, fp_, fp_id_, fact_);
	}
	do_afterwork(env, ctx_);
}

void JfpExec::exec_struct(const EnvironPtr &env, const void *sub)
{
	Context child_ctx;
	// map sub parameters
	auto params_count=sub_params_count(sub);
	assert(params_count==bi_args_count(cf_));

	for (auto i=0u; i<params_count; i++) {
		auto par_type=sub_partype(sub, i);
		auto par_name=sub_parname(sub, i);
		auto arg=bi_arg(cf_, i);
		ValuePtr arg_val;
		if (par_type==PARAM_NAME) {
			assert(expr_type(arg)==EXPR_ID);
			arg_val=ValuePtr(new NameValue(ctx_.eval_ref(
				expr_id_value(arg))));
		} else {
			arg_val=ctx_.eval(arg);
		}
		child_ctx.set_param(par_name, ctx_.cast(par_type, arg_val));
	}

	spawn_body(env, sub_body(sub), sub_rules(sub), child_ctx, loc_,
		locators_, fp_, fp_id_, fact_);

	do_afterwork(env, ctx_);
}

void JfpExec::spawn_body(const EnvironPtr &env, const void *body,
		const void *rules, const Context &base_ctx,
		const LocatorPtr &cur_loc, const std::map<Id, WORD> &my_locators,
		const void *fp, const Id &fp_id, Factory &fact)
{
	auto locators__=my_locators;
	NOTE("spawn_body =>* " 
		+ std::to_string(body_size(body)));
	Context ctx; ctx=base_ctx;
	// create local dfs
	std::map<Name, Id> dfs_names;

	auto bi_count=body_size(body);
	for (auto i=0u; i<bi_count; i++) {
		const void *bi=body_item(body, i);
		if (bi_type(bi)==BI_DFS) {
			// Gather & create dfs names
			auto dfs_count=bi_dfs_count(bi);
			for (auto j=0u; j<dfs_count; j++) {
				Name sname(bi_df(bi, j));
				if (dfs_names.find(sname)!=dfs_names.end()) {
					fprintf(stderr, "JfpExec::spawn_body: duplicate "
						"df identifier (%s)\n",
						sname.c_str());
					abort();
				}
				dfs_names[sname]=env->create_id(sname);
			}
		} else {
			// create cfs names
			if (bi_type(bi)==BI_EXEC) {
				const void *ref=bi_id(bi);
				Name name(ref_basename(ref));
				ctx.set_name(name, env->create_id(name));
			}
		}
	}

	for (auto el : dfs_names) {
		ctx.set_name(el.first, el.second);
	} 

	// set global locators
	for (auto it : CF::get_global_locators(rules, ctx)) {
		if (locators__.find(it.first)!=locators__.end()) {
			ABORT("Locator redefinition for " + it.first.to_string());
		}
		locators__[it.first]=DP(fp, it.second);
	}

	// set local locators
	for (auto i=0u; i<bi_count; i++) {
		const void *bi=body_item(body, i);
		if (bi_type(bi)==BI_EXEC) {
			// create cfs names
			const void *ref=bi_id(bi);
			Id prefix=ctx.eval_ref(ref, 0);
			if(locators__.find(prefix)==locators__.end()) {
				locators__[prefix]=DP(fp, CF::get_loc_spec(bi));
			}
		}
	}

	// spawn
	for (auto i=0u; i<bi_count; i++) {
		const void *bi=body_item(body, i);
		if (bi_type(bi)!=BI_DFS) {
			Id item_id;
			if (bi_type(bi)==BI_EXEC) {
				item_id=ctx.eval_ref(bi_id(bi));
			} else {
				item_id=env->create_id("_anon_" 
					+ std::to_string(bi_type(bi)));
			}
			LocatorPtr loc=CF::get_locator(bi, cur_loc, ctx);
			if (!loc) {
				loc=get_global_locator(item_id, env, ctx, locators__, fp);
			}
			if (!loc) {
				ABORT("Failed to deduce locator");
				// Or try to submit a trivial one?
			}
			WORD bi_ofs=DP(fp, bi);
			JfpExec *item=new JfpExec(fp_id, item_id, bi_ofs, loc,
				fact);
			item->cf_=bi;
			item->fp_=fp;
			TaskPtr task(item);
			// push context
			init_child_context(item, ctx, locators__);
			env->submit(task);
		}
	}
}

void JfpExec::exec_extern(const EnvironPtr &env, const void *sub)
{
	assert(bi_type(cf_)==BI_EXEC);
	assert(sub_params_count(sub)==bi_args_count(cf_));

	std::vector<CodeLib::Argument> args;
	auto params_count=sub_params_count(sub);
	
	for (auto i=0u; i<params_count; i++) {
		auto param=sub_partype(sub, i);
		auto arg=bi_arg(cf_, i);

		if (param==PARAM_NAME) {
			args.push_back(std::make_tuple(
				Reference,
				ValuePtr(nullptr),
				ctx_.eval_ref(expr_id_value(arg))
			));
		} else if (param==PARAM_VALUE) {
			args.push_back(std::make_tuple(
				Custom,
				ctx_.cast(PARAM_VALUE, ctx_.eval(arg)),
				ctx_.eval_ref(expr_id_value(arg))
			));
		} else {
			auto val=ctx_.cast(param, ctx_.eval(arg));
			args.push_back(std::make_tuple(
				val->type(),
				val, Id()));
		}
	}

	NOTE("EXEC_EXTERN " + cf_id_.to_string());
	env->exec_extern(sub_code(sub), args);
	NOTE("EXEC_EXTERN done" + cf_id_.to_string());

	for (auto i=0u; i<params_count; i++) {
		auto param=sub_partype(sub, i);
		auto arg=bi_arg(cf_, i);

		if (param==PARAM_VALUE) {
			// check if grabbed:
			if (!std::get<1>(args[i])) {
				ABORT("Input was grabbed, gotta do something here: "
					+ std::get<2>(args[i]).to_string());
			}
		} else if (param==PARAM_NAME) {
			if (std::get<1>(args[i])) {
				df_computed(env, ctx_.eval_ref(expr_id_value(arg)),
					std::get<1>(args[i]));
			} else {
				fprintf(stderr, "extern %s: output parameter %d not set\n",
					sub_code(sub), (int)i);
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
	for (auto push : CF::get_afterpushes(cf_, ctx)) {
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
	//Afterdels
	for (auto dfid : CF::get_afterdels(cf_, ctx)) {
		NOTE("AFTERDEL " + dfid.to_string());
		TaskPtr task(new DelDf(dfid));;
		NodeId next_rank=glocate_next_node(dfid, env, ctx_);
		if (next_rank==env->comm().get_rank()) {
			env->submit(task);
		} else {
			env->submit(TaskPtr(new Delivery(LocatorPtr(
				new CyclicLocator(next_rank)), task)));
		}
	}
	//Aftersets
	for (auto dfid : CF::get_aftersets(cf_, ctx)) {
		df_computed(env, dfid, IntValue::create(1));
	}
	NOTE("AFTERWORK DONE: " + cf_id_.to_string());
}
void JfpExec::df_computed(const EnvironPtr &env, const Id &id,
	const ValuePtr &val)
{
	NOTE("DF COMPUTED: " + id.to_string() + ": " + val->to_string());

	ctx_.set_df(id, val);

	// store if requestable
	if (CF::is_df_requested(cf_, ctx_, id)) {
		NodeId store_node=glocate_next_node(id, env, ctx_);
		int req_count;
		if (CF::is_requested_unlimited(cf_, ctx_, id)) {
			req_count=-1;
		} else {
			req_count=CF::get_requests_count(cf_, ctx_, id);
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

void JfpExec::init_child_context(JfpExec *child, const Context &ctx,
	const std::map<Id, WORD> &locators)
{
	// TODO optimize here:
	child->ctx_.pull_names(ctx); // maybe too much, but generally every
	// body item can address any other body item
	child->ctx_.pull_params(ctx); // maybe too much to clone?

	// maybe too much? only import adressed names
	child->locators_=locators;

	switch (bi_type(child->cf_)) {
		case BI_EXEC: {
			auto args_count=bi_args_count(child->cf_);
			if (args_count==0) {
				return;
			} else {
				for (auto i=0u; i<args_count; i++) {
					const void *arg=bi_arg(child->cf_, i);
					init_child_context_arg(child, arg, ctx);
				}
			}
			break;
		}
		case BI_IF:
		case BI_FOR:
		case BI_WHILE:
			child->ctx_=ctx;
			break;
		default:
			bi_print(child->fp_, child->cf_);
			ABORT("type not supported: " 
				+ std::to_string(bi_type(child->cf_)));
	}
}

void JfpExec::init_child_context_arg(JfpExec *child,
	const void *arg, const Context &ctx)
{
	switch (expr_type(arg)) {
		case EXPR_ICONST:
		case EXPR_RCONST:
		case EXPR_SCONST:
			break; // do nothing
		case EXPR_ID:
			{
				auto ref=expr_id_value(arg);
				Name name(ref_basename(ref));
				if (!child->ctx_.has_param(name)) {
					child->ctx_.pull_name(name, ctx);
				}

				auto idx_count=ref_idx_count(ref);
				for(WORD i=0; i<idx_count; i++) {
					auto idx=ref_idx(ref, i);
					init_child_context_arg(child, idx, ctx);
				}
				break;
			}
		case EXPR_ICAST:
		case EXPR_RCAST:
		case EXPR_SCAST:
			init_child_context_arg(child, expr_subexpr(arg), ctx);
			break;
		case EXPR_PLUS:
		case EXPR_MINUS:
		case EXPR_MUL:
		case EXPR_DIV:
		case EXPR_MOD:
			{
				auto op_count=expr_op_count(arg);
				for (auto i=0u; i<op_count; i++) {
					init_child_context_arg(child, expr_op(arg, i), ctx);
				}
			}
			break;
		default:
			expr_print(arg);
			fprintf(stderr, "JfpExec::init_child_context_arg: arg type"
				" not supported: %d\n", (int)expr_type(arg));
			ABORT("Type not supported");
	}
//	} else if (arg["type"]=="icast") {
//		init_child_context_arg(child, arg["expr"], ctx);
//	} else if (arg["type"]=="+" || arg["type"]=="/"
//			|| arg["type"]=="*" || arg["type"]=="-"
//			|| arg["type"]=="%"
//	) {
//		for (auto op : arg["operands"]) {
//			init_child_context_arg(child, op, ctx);
//		}
}
/*
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
		} else if (rule["ruletype"]=="indexed_setdfs") {
			assert(rule.find("dfs")!=rule.end());
		} else {
			fprintf(stderr, "rule: %s\n", rule.dump(2).c_str());
			ABORT("Unknown rule type: " + rule["ruletype"].dump());
		}
	}
}

*/
LocatorPtr JfpExec::get_global_locator(const Id &id, const EnvironPtr &env,
	const Context &ctx, const std::map<Id, WORD> &locators, const void *fp)
{
	Id prefix=id;
	while (prefix.size()>=2) {
		auto it=locators.find(prefix);
		if (it!=locators.end()) {
			const void *loc=OFS(fp, it->second, const void);
			return CF::get_global_locator(loc,
				std::vector<int>(id.begin()+prefix.size(), id.end()), ctx);
		}
		prefix.pop_back();
	}
	ABORT("No locator found for: " + id.to_string());
}

NodeId JfpExec::glocate_next_node(const Id &dfid, const EnvironPtr &env,
	const Context &ctx)
{
	auto gloc=get_global_locator(dfid, env, ctx, locators_, fp_);
	auto res=gloc->get_next_node(env->comm());
	return res;
}

JfpReg::JfpReg(BufferPtr &buf, Factory &)
{
	fp_id_=Id(buf);
	fp_buf_=Buffer::popBuffer(buf);
	rptr_=RPtr(buf);
}

void JfpReg::run(const EnvironPtr &env)
{
	auto ret=env->set(fp_id_, ValuePtr(CustomValue::create_copy(
		fp_buf_->getData(), fp_buf_->getSize())));
	assert(!ret);

	env->submit(TaskPtr(new MonitorSignal(rptr_, {})));
}
