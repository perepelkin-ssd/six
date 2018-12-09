#include "jfp.h"

#include "common.h"
#include "tasks.h"

JfpExec::JfpExec(const Id &fp_id, const Id &cf_id, const std::string &jdump)
	: fp_id_(fp_id), cf_id_(cf_id), j_(nlohmann::json::parse(jdump))
{
}

JfpExec::JfpExec(BufferPtr &buf, Factory &fact)
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
}

void JfpExec::run(const EnvironPtr &env)
{
	fp_=&dynamic_cast<const JsonValue*>(env->get(fp_id_).get())->value();
	
	NodeId next_node=get_next_node(env);

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
}


std::string JfpExec::to_string() const
{
	return "JfpExec(" + cf_id_.to_string() + ")";
}

void JfpExec::resolve_args(const EnvironPtr &env)
{
	auto args=get_args();
	for (auto a : args) {
		printf("ARG: %s\n", a.to_string().c_str());
	}
	if (!args.empty()) {
		NIMPL
	}

	// All args resolved, execute
	exec(env);
}

std::set<Id> JfpExec::get_args()
{
	if (j_["type"]=="exec") {
		return get_args_exec();
	} else {
		fprintf(stderr, "JfpExec::get_args: unsupported cf type: %s\n",
			j_["type"].dump().c_str());
		abort();
	}
}

std::set<Id> JfpExec::get_args_exec()
{
	auto sub=fp()[j_["code"].get<std::string>()];

	if (sub["args"].size()==0) {
		return {};
	} else {
		fprintf(stderr, "JfpExec::get_args_exec: NIMPL\n%s",
			sub.dump(2).c_str());
		NIMPL
	}
}


NodeId JfpExec::get_next_node(const EnvironPtr &env)
{
	assert(cf_id_.size()>=2);
	return (cf_id_[1]) % env->comm().get_size();
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
		exec_extern(env, sub);
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
			for (auto name : bi["names"]) {
				if (dfs_names.find(name)!=dfs_names.end()) {
					fprintf(stderr, "JfpExec::exec_struct: duplicate "
						"df identifier (%s) in sub %s\n",
						name.get<std::string>().c_str(),
						j_["code"].get<std::string>().c_str());
					abort();
				}
				dfs_names[name]=env->create_id(name);
				printf("name id created: %s <= %s\n",
					name.get<std::string>().c_str(),
					dfs_names[name].to_string().c_str());
			}
		}
	}

	for (auto bi : sub["body"]) {
		if (bi["type"]!="dfs") {
			Id item_id;
			assert(bi.find("id") != bi.end());
			if (bi["id"].size()==1) {
				item_id=env->create_id(bi["id"][0].get<std::string>());
			} else {
				printf("%s\n", bi["id"].dump(2).c_str());
				NIMPL
			}
			JfpExec *item=new JfpExec(fp_id_, item_id, bi.dump());
			TaskPtr task(item);
			// push context
			init_child_context(item);
			env->submit(task);
		}
	}
}

void JfpExec::exec_extern(const EnvironPtr &env, const nlohmann::json &ext)
{
	// Algorithm: eval args & call sub; return args save as dfs.
	if (ext["args"].empty()) {
		env->exec_extern(ext["code"].get<std::string>());
	} else {
		fprintf(stderr, "JfpExec::exec_extern: args not supported in %s\n",
			ext.dump(2).c_str());
		NIMPL
	}
}

void JfpExec::init_child_context(JfpExec *child)
{
	if (child->j_["type"]=="exec") {
		if (child->j_["args"].empty()) {
			return;
		} else {
			fprintf(stderr, "JfpExec::init_child_context: %s\n",
				child->j_.dump(2).c_str());
			NIMPL
		}
	} else {
		fprintf(stderr, "JfpExec::init_child_context: type not supported,"
			" type=%s, child=%s\n", child->j_["type"].get<std::string>()
				.c_str(), child->j_.dump(2).c_str());
		abort();
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
