#include "context.h"

#include "common.h"

Context::Context(BufferPtr &buf, Factory &fact)
{
	// names_
	size_t count=Buffer::pop<size_t>(buf);
	for (auto i=0u; i<count; i++) {
		auto key=Buffer::popString(buf);
		auto val=Id(buf);
		_set_name(key, val);
	}

	// params_
	count=Buffer::pop<size_t>(buf);
	for (auto i=0u; i<count; i++) {
		auto key=Buffer::popString(buf);
		auto val=fact.pop<Value>(buf);
		_set_param(key, val);
	}

	// dfs_
	count=Buffer::pop<size_t>(buf);
	for (auto i=0u; i<count; i++) {
		auto key=Id(buf);
		auto val=fact.pop<Value>(buf);
		_set_df(key, val);
	}
}

Context &Context::operator=(const Context &ctx)
{
	std::lock_guard<std::mutex> lk(m_);
	names_=ctx.names_;
	params_=ctx.params_;
	dfs_=ctx.dfs_;
	return *this;
}


void Context::set_name(const Name &name, const Id &id)
{
	std::lock_guard<std::mutex> lk(m_);
	_set_name(name, id);
}

void Context::set_param(const Name &name, const ValuePtr &val)
{
	std::lock_guard<std::mutex> lk(m_);
	_set_param(name, val);
}

void Context::set_df(const Id &id, const ValuePtr &val)
{
	std::lock_guard<std::mutex> lk(m_);
	_set_df(id, val);
}

bool Context::has_name(const Name &name) const
{
	std::lock_guard<std::mutex> lk(m_);
	return names_.find(name)!=names_.end();
}

bool Context::has_param(const Name &name) const
{
	std::lock_guard<std::mutex> lk(m_);
	return params_.find(name)!=params_.end();
}

bool Context::has_df(const Id &id) const
{
	std::lock_guard<std::mutex> lk(m_);
	return dfs_.find(id)!=dfs_.end();
}

Id Context::get_name(const Name &name) const
{
	std::lock_guard<std::mutex> lk(m_);
	return _get_name(name);
}

ValuePtr Context::get_param(const Name &name) const
{
	std::lock_guard<std::mutex> lk(m_);
	return _get_param(name);
}

ValuePtr Context::get_df(const Id &id) const
{
	std::lock_guard<std::mutex> lk(m_);
	return _get_df(id);
}

ValuePtr Context::eval(const json &expr) const
{
	std::lock_guard<std::mutex> lk(m_);
	return _eval(expr);
}

Id Context::eval_ref(const json &ref) const
{
	std::lock_guard<std::mutex> lk(m_);
	return _eval_ref(ref);
}

void Context::pull_name(const Name &name, const Context &ctx)
{
	std::lock_guard<std::mutex> lk(m_);
	_pull_name(name, ctx);
}

void Context::_set_name(const Name &name, const Id &id)
{
	if (names_.find(name)!=names_.end()) {
		fprintf(stderr, "Context::_set_name: name %s redefinition attempt "
			"from %s to %s\n", name.c_str(),
			names_[name].to_string().c_str(), id.to_string().c_str());
		abort();
	}

	names_[name]=id;
}

void Context::_set_param(const Name &name, const ValuePtr &val)
{
	if (params_.find(name)!=params_.end()) {
		fprintf(stderr, "Context::_set_param: param %s redefinition "
			"attempt from %s to %s\n", name.c_str(),
			params_[name]->to_string().c_str(), val->to_string().c_str());
		abort();
	}

	params_[name]=val;
}

void Context::_set_df(const Id &id, const ValuePtr &val)
{
	if (dfs_.find(id)!=dfs_.end()) {
		fprintf(stderr, "Context::_set_df: df %s redefinition attempt "
			"from %s to %s\n", id.to_string().c_str(),
			dfs_[id]->to_string().c_str(), val->to_string().c_str());
		abort();
	}

	dfs_[id]=val;
}

Id Context::_get_name(const Name &name) const
{
	auto it=names_.find(name);
	if (it==names_.end()) {
		fprintf(stderr, "Context::_get_name: not found: %s\n",
			name.c_str());
		abort();
	} else {
		return it->second;
	}
}

ValuePtr Context::_get_param(const Name &name) const
{
	auto it=params_.find(name);
	if (it==params_.end()) {
		fprintf(stderr, "Context::_get_param: not found: %s\n",
			name.c_str());
		abort();
	} else {
		return it->second;
	}
}

ValuePtr Context::_get_df(const Id &id) const
{
	auto it=dfs_.find(id);
	if (it==dfs_.end()) {
		fprintf(stderr, "Context::_get_df: not found: %s\n",
			id.to_string().c_str());
		abort();
	} else {
		return it->second;
	}
}


ValuePtr Context::_eval(const json &expr) const
{
	if (expr["type"]=="iconst") {
		return ValuePtr(new IntValue(expr["value"].get<int>()));
	} else if (expr["type"]=="id") {
		return _get_df(_eval_ref(expr["ref"]));
	} else {
		fprintf(stderr, "JfpExec::eval: %s\n", expr.dump(2).c_str());
		NIMPL
	}
}

Id Context::_eval_ref(const json &ref) const
{
	Name ref_name=ref[0].get<std::string>();

	auto it=names_.find(ref_name);

	if (it==names_.end()) {
		fprintf(stderr, "Context::eval_ref: name %s is missing in ref %s\n",
			ref_name.c_str(), ref.dump(2).c_str());
		abort();
	}

	Id id=it->second;

	for (auto i=1u; i<ref.size(); i++) {
		id.push_back((int)(*_eval(ref[i])));
	}

	return id;
}

void Context::_pull_name(const Name &name, const Context &ctx)
{
	auto id=ctx.get_name(name);
	auto it=names_.find(name);
	if (it==names_.end()) {
		_set_name(name, id);
	} else if (id!=names_[name]) {
		fprintf(stderr, "Context::_get_name: name %s is already set "
			"to %s, but got %s from the other context\n",
				name.c_str(), names_[name].to_string().c_str(),
				id.to_string().c_str());
		abort();
	}
}

void Context::serialize(Buffers &bufs) const
{
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
