// TODO: use locators instead of dfid hash
#include "jfp.h"

#include "common.h"
#include "remote_monitor.h"
#include "tasks.h"

JfpExec::JfpExec(const Id &fp_id, const Id &cf_id, const std::string &jdump,
	Factory &fact)
	: fact_(fact), fp_id_(fp_id), cf_id_(cf_id),
		j_(nlohmann::json::parse(jdump))
{
}

JfpExec::JfpExec(BufferPtr &buf, Factory &fact)
	:fact_(fact)
{
	fp_id_=Id(buf);
	cf_id_=Id(buf);
	j_=nlohmann::json::parse(Buffer::popString(buf));

	// names_
	size_t count=Buffer::pop<size_t>(buf);
	for (auto i=0u; i<count; i++) {
		auto key=Buffer::popString(buf);
		auto val=Id(buf);
		assert(names_.find(key)==names_.end());
		names_[key]=val;
	}

	// params_
	count=Buffer::pop<size_t>(buf);
	for (auto i=0u; i<count; i++) {
		auto key=Buffer::popString(buf);
		auto val=fact.pop<Value>(buf);
		assert(params_.find(key)==params_.end());
		params_[key]=val;
	}

	// dfs_
	count=Buffer::pop<size_t>(buf);
	for (auto i=0u; i<count; i++) {
		auto key=Id(buf);
		auto val=fact.pop<Value>(buf);
		assert(dfs_.find(key)==dfs_.end());
		dfs_[key]=val;
	}

	// dfpush_simple_noidx_
	count=Buffer::pop<size_t>(buf);
	for (auto i=0u; i<count; i++) {
		auto df=Id(buf);
		auto cf=Id(buf);
		auto rec=std::make_pair(df, cf);
		assert(dfpush_simple_noidx_.find(rec)==dfpush_simple_noidx_.end());
		dfpush_simple_noidx_.insert(rec);
	}

	// dfreqcount_simple_noidx_
	count=Buffer::pop<size_t>(buf);
	for (auto i=0u; i<count; i++) {
		auto df=Id(buf);
		auto count=Buffer::pop<size_t>(buf);
		assert(dfreqcount_simple_noidx_.find(df)
			==dfreqcount_simple_noidx_.end());
		dfreqcount_simple_noidx_[df]=count;
	}
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

	// names_
	bufs.push_back(Buffer::create<size_t>(names_.size()));
	for (auto el : names_) {
		bufs.push_back(Buffer::create(el.first));
		el.second.serialize(bufs);
	}

	// params_
	bufs.push_back(Buffer::create<size_t>(params_.size()));
	for (auto el : params_) {
		bufs.push_back(Buffer::create(el.first));
		el.second->serialize(bufs);
	}

	// dfs_
	bufs.push_back(Buffer::create<size_t>(dfs_.size()));
	for (auto el : dfs_) {
		el.first.serialize(bufs);
		el.second->serialize(bufs);
	}

	// dfpush_simple_noidx_
	bufs.push_back(Buffer::create<size_t>(dfpush_simple_noidx_.size()));
	for (auto el : dfpush_simple_noidx_) {
		el.first.serialize(bufs);
		el.second.serialize(bufs);
	}

	//dfreqcount_simple_noidx_
	bufs.push_back(Buffer::create<size_t>(dfreqcount_simple_noidx_.size()));
	for (auto el : dfreqcount_simple_noidx_) {
		el.first.serialize(bufs);
		bufs.push_back(Buffer::create<size_t>(el.second));
	}
}


std::string JfpExec::to_string() const
{
	return "JfpExec(" + cf_id_.to_string() + ")";
}

void JfpExec::resolve_args(const EnvironPtr &env)
{
	pushed_flag_=false;
	bool requested_flag=false;

	auto deps=get_deps();
	for (auto d : deps) {
		printf("DEPENDENCY: %s for %s\n", d.to_string().c_str(),
			cf_id_.to_string().c_str());
		auto push_rec=std::make_pair(d, cf_id_);
		if (dfpush_simple_noidx_.find(push_rec)
				==dfpush_simple_noidx_.end()) {
			printf("DEPENDENCY: %s for %s - REQUESTING\n",
				d.to_string().c_str(), cf_id_.to_string().c_str());
			NodeId df_node=get_next_node(env, d);
			if (df_node==env->comm().get_rank()) {
				env->df_requester().request(d, [this, d, env](
						const ValuePtr &val){
					assert(dfs_.find(d)==dfs_.end());
					dfs_[d]=val;
					check_exec(env);
				});
			} else {
				auto rptr=RPtr(env->comm().get_rank(), remote_callback(
						[this, d, env](BufferPtr &buf) {
					ValuePtr val=fact_.pop<Value>(buf);
					assert(dfs_.find(d)==dfs_.end());
					dfs_[d]=val;
					check_exec(env);
				}));
				env->submit(TaskPtr(new Delivery(LocatorPtr(
					new CyclicLocator(df_node)), TaskPtr(new RequestDf(
					d, LocatorPtr(new CyclicLocator(env->comm()
					.get_rank())), rptr)))));
			}
			requested_flag=true;
		} else {
			pushed_flag_=true;
		}
	}
	if (pushed_flag_) {
		printf("\tOPEN %s\n", cf_id_.to_string().c_str());
		env->df_pusher().open(cf_id_, [this, env](const Id &dfid,
				const ValuePtr &val) {
			assert(dfs_.find(dfid)==dfs_.end());
			dfs_[dfid]=val;
			check_exec(env);
		});
	}
	printf("%s %s %s\n", cf_id_.to_string().c_str(), (pushed_flag_? "P": "!p"),
		(requested_flag? "R": "!r"));
	if (!pushed_flag_ && !requested_flag) {
		exec(env);
	}
}

std::set<Id> JfpExec::get_deps()
{
	std::set<Id> res;
	if (j_["type"]=="exec") {
		for (auto i=0u; i<j_["args"].size(); i++) {
			auto arg=j_["args"][i];
			auto param_spec=fp()[j_["code"].get<std::string>()]["args"][i];
			bool is_input=param_spec["type"]!="name";
			if (is_input) {
				extract_deps(res, arg);
			} else {
				extract_name_deps(res, arg["ref"]);
			}
		}
		return res;
	} else {
		fprintf(stderr, "JfpExec::get_args: unsupported cf type: %s\n",
			j_["type"].dump().c_str());
		abort();
	}
}

void JfpExec::extract_deps(std::set<Id> &deps, const nlohmann::json &expr)
{
	if (expr["type"]=="id") {
		extract_deps_idexpr(deps, expr);
	} else if (expr["type"]=="iconst") {
		// no deps
	} else {
		fprintf(stderr, "JfpExec::extract_deps: %s\n",
			expr.dump(2).c_str());
		NIMPL
	}
}

void JfpExec::extract_name_deps(std::set<Id> &deps,
	const nlohmann::json &ref)
{
	for (auto i=1u; i<ref.size(); i++) {
		extract_deps(deps, ref[i]);
	}
}

void JfpExec::extract_deps_idexpr(std::set<Id> &deps,
	const nlohmann::json &expr)
{
	assert(expr["type"]=="id");

	bool indices_evaluatable=true;

	for (auto i=1u; i<expr["ref"].size(); i++) {
		std::set<Id> arg_deps;
		extract_deps(arg_deps, expr["ref"][i]);
		if (!arg_deps.empty()) {
			indices_evaluatable=false;
			deps.insert(arg_deps.begin(), arg_deps.end());
		}
	}

	if (!indices_evaluatable) {
		return;
	}

	Id id=eval_ref(expr["ref"]);
	if (dfs_.find(id)==dfs_.end()) {
		deps.insert(id);
	}
}

Id JfpExec::eval_ref(const nlohmann::json &ref)
{
	assert(names_.find(ref[0].get<std::string>())!=names_.end());
	Id id=names_[ref[0].get<std::string>()];

	for (auto i=1u; i<ref.size(); i++) {
		id.push_back((int)(*eval(ref[i])));
	}

	return id;
}

NodeId JfpExec::get_next_node(const EnvironPtr &env, const Id &id)
{
	assert(id.size()>=2);
	return (id[1]) % env->comm().get_size();
}

void JfpExec::check_exec(const EnvironPtr &env)
{
	if (get_deps().empty()) {
		exec(env);

		if (pushed_flag_) {
			env->df_pusher().close(cf_id_);
		}
	}
}

void JfpExec::exec(const EnvironPtr &env)
{
	printf("EXEC %s\n", cf_id_.to_string().c_str());
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

void JfpExec::exec_struct(const EnvironPtr &env, const nlohmann::json &sub)
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
				printf("\tDFNAME %s << %s\n", sname.c_str(),
					dfs_names[sname].to_string().c_str());
			}
		} else if (bi["type"]!="sub_rules") {
			// create cfs names
			assert(bi.find("id")!=bi.end());
			Name name=bi["id"][0].get<std::string>();
			if (names_.find(name)!=names_.end()) {
				fprintf(stderr, "JfpExec::exec_struct: duplicate name:"
					" %s\n", name.c_str());
				abort();
			}
			names_[name]=env->create_id(name);
			printf("\tCFNAME %s << %s\n", name.c_str(),
				names_[name].to_string().c_str());
		}
	}

	for (auto el : dfs_names) {
		if (names_.find(el.first)!=names_.end()) {
			fprintf(stderr, "JfpExec::exec_struct: duplicate name:"
				" %s\n", el.first.c_str());
			abort();
		}
		names_[el.first]=el.second;
		printf("\tDFNAME fwd %s << %s\n", el.first.c_str(),
			el.second.to_string().c_str());
	}

	for (auto bi : sub["body"]) {
		if (bi["type"]=="sub_rules") {
			// push table
			if (bi.find("push_table")!=bi.end()) {
				for (auto el : bi["push_table"]) {
					if (el["type"]=="simple_noidx") {
						assert(names_.find(el["df"])!=names_.end());
						assert(names_.find(el["cf"])!=names_.end());
						printf("dfpush insert %s=>%s in %s\n",
							names_[el["df"]].to_string().c_str(),
							names_[el["cf"]].to_string().c_str(),
							cf_id_.to_string().c_str());
						dfpush_simple_noidx_.insert(std::make_pair(
							names_[el["df"]], names_[el["cf"]]));
					} else {
						fprintf(stderr, "JfpExec::exec_struct: "
							"push table entry not supported: %s\n",
							el["type"].dump().c_str());
						abort();
					}
				}
			}
			// req_counts
			if (bi.find("req_counts")!=bi.end()) {
				for (auto el : bi["req_counts"]) {
					if (el["type"]=="simple_noidx") {
						assert(names_.find(el["df"])!=names_.end());
						Id dfid=names_[el["df"]];
						assert(dfreqcount_simple_noidx_.find(dfid)
							==dfreqcount_simple_noidx_.end());
						dfreqcount_simple_noidx_[dfid]=
							el["count"].get<int>();
					} else {
						fprintf(stderr, "JfpExec::exec_struct: "
							"req_count entry not supported: %s\n",
							el.dump().c_str());
						abort();
					}
				}
			}
		}
	}
	for (auto bi : sub["body"]) {
		if (bi["type"]!="dfs" && bi["type"]!="sub_rules") {
			Id item_id;
			assert(bi.find("id") != bi.end());
			item_id=Id(names_[bi["id"][0]]);
			for (auto i=1u; i<bi["id"].size(); i++) {
				item_id.push_back((int)(*eval(bi["id"][i])));
			}
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
			args.push_back(eval(j_["args"][i]));
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
}

void JfpExec::df_computed(const EnvironPtr &env, const nlohmann::json &ref,
	const ValuePtr &val)
{
	Id id=eval_ref(ref);

	printf("%d: DF COMPUTED: %s = %s, %d by %s\n",
		(int)env->comm().get_rank(),
		id.to_string().c_str(), val->to_string().c_str(),
		(int)dfpush_simple_noidx_.size(),
		cf_id_.to_string().c_str());

	std::set<std::pair<Id, Id> > to_delete;
	// push
	for (auto el : dfpush_simple_noidx_) {
		printf("%s ?= %s\n", el.first.to_string().c_str(),
			id.to_string().c_str());
		if (el.first==id) {
			Id dest_cfid=el.second;
			NodeId next_rank=get_next_node(env, dest_cfid);
			if (next_rank==env->comm().get_rank()) {
				printf("%d: DF PUSHING LOCAL: %s = %s\n",
					(int)env->comm().get_rank(),
					id.to_string().c_str(), val->to_string().c_str());
				env->df_pusher().push(dest_cfid, id, val);
			} else {
				printf("%d: DF PUSHING DELIVERY to %d: %s = %s >> %s\n",
					(int)env->comm().get_rank(), (int)next_rank,
					id.to_string().c_str(), val->to_string().c_str(),
					dest_cfid.to_string().c_str());

				env->submit(TaskPtr(new Delivery(LocatorPtr(
					new CyclicLocator(next_rank)), TaskPtr(
					new SubmitDfToCf(id, val, dest_cfid)))));
			}
			to_delete.insert(el);
		}
	}

	for (auto el : to_delete) {
		dfpush_simple_noidx_.erase(el);
	}

	// store requested
	if (dfreqcount_simple_noidx_.find(id)!=dfreqcount_simple_noidx_.end()) {
		printf("\tREQUEST COUNT for %s = %d\n",
			id.to_string().c_str(), (int)dfreqcount_simple_noidx_[id]);
		NodeId store_node=get_next_node(env, id);
		if (store_node==env->comm().get_rank()) {
			std::shared_ptr<size_t> counter(
				new size_t(dfreqcount_simple_noidx_[id]));
			env->df_requester().put(id, val, [counter, env, id](){
				assert (*counter>0);
				(*counter)--;

				if (*counter==0) {
					env->df_requester().del(id);
				}
			});
		} else {
			env->submit(TaskPtr(new Delivery(LocatorPtr(
				new CyclicLocator(store_node)), TaskPtr(
				new StoreDf(id, val, nullptr, 
				dfreqcount_simple_noidx_[id])))));
		}
	}
}

void JfpExec::init_child_context(JfpExec *child)
{
	printf("warning: pushing too much context %s<<%s, %d\n",
		child->cf_id_.to_string().c_str(),
		cf_id_.to_string().c_str(), (int)dfpush_simple_noidx_.size());
	child->dfpush_simple_noidx_=dfpush_simple_noidx_;
	child->dfreqcount_simple_noidx_=dfreqcount_simple_noidx_;

	if (child->j_["type"]=="exec") {
		if (child->j_["args"].empty()) {
			return;
		} else {
			for (auto arg : child->j_["args"]) {
				init_child_context_arg(child, arg);
			}
		}
	} else {
		fprintf(stderr, "JfpExec::init_child_context: type not supported,"
			" type=%s, child=%s\n", child->j_["type"].get<std::string>()
				.c_str(), child->j_.dump(2).c_str());
		abort();
	}
}

void JfpExec::init_child_context_arg(JfpExec *child,
	const nlohmann::json &arg)
{
	if (arg["type"]=="id") {
		Name name=arg["ref"][0].get<std::string>();
		assert(names_.find(name)!=names_.end());
		assert(child->names_.find(name)==child->names_.end()
			|| child->names_[name]==names_[name]);
		child->names_[name]=names_[name];

		for (auto i=1u; i<arg["ref"].size(); i++) {
			init_child_context_arg(child, arg["ref"][i]);
		}
	} else if (arg["type"]=="iconst") {
		// do nothing
	} else {
		fprintf(stderr, "JfpExec::init_child_context: %s\n",
			arg.dump(2).c_str());
		NIMPL
	}
}

ValuePtr JfpExec::eval(const nlohmann::json &expr)
{
	if (expr["type"]=="iconst") {
		return ValuePtr(new IntValue(expr["value"].get<int>()));
	} else if (expr["type"]=="id") {
		Id id=eval_ref(expr["ref"]);
		if (dfs_.find(id)!=dfs_.end()) {
			return dfs_[id];
		} else {
			fprintf(stderr, "JfpExec::eval: df not present: %s\n",
				id.to_string().c_str());
			abort();
		}
	} else {
		fprintf(stderr, "JfpExec::eval: %s\n", expr.dump(2).c_str());
		NIMPL
	}
}

JfpReg::JfpReg(BufferPtr &buf, Factory &)
{
	fp_id_=Id(buf);
	auto s=Buffer::popString(buf);
	j_=nlohmann::json::parse(s);
	rptr_=RPtr(buf);
}

void JfpReg::run(const EnvironPtr &env)
{
	auto ret=env->set(fp_id_, ValuePtr(new JsonValue(j_)));
	assert(!ret);

	env->submit(TaskPtr(new MonitorSignal(rptr_, {})));
}
