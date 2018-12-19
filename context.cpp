#include "context.h"

#include <sstream>

#include "common.h"

Context::Context(const Context &ctx)
{
	*this=ctx;
}

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

void Context::set_param(const Name &name, const ValuePtr &val,
	bool allow_override)
{
	std::lock_guard<std::mutex> lk(m_);
	_set_param(name, val, allow_override);
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

bool Context::can_eval(const json &expr) const
{
	std::lock_guard<std::mutex> lk(m_);
	return _can_eval(expr);
}

bool Context::can_eval_ref(const json &ref) const
{
	std::lock_guard<std::mutex> lk(m_);
	return _can_eval_ref(ref);
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

void Context::pull_names(const Context &ctx)
{
	std::lock_guard<std::mutex> lk(m_);
	_pull_names(ctx);
}

void Context::pull_params(const Context &ctx)
{
	std::lock_guard<std::mutex> lk(m_);
	_pull_params(ctx);
}

void Context::_set_name(const Name &name, const Id &id)
{
	if (names_.find(name)!=names_.end()) {
		fprintf(stderr, "Context::_set_name: name %s redefinition attempt "
			"from %s to %s\n", name.c_str(),
			names_[name].to_string().c_str(), id.to_string().c_str());
		ABORT("");
	}

	names_[name]=id;
}

void Context::_set_param(const Name &name, const ValuePtr &val,
	bool allow_override)
{
	if (params_.find(name)!=params_.end() && !allow_override) {
		fprintf(stderr, "Context::_set_param: param %s redefinition "
			"attempt from %s to %s\n", name.c_str(),
			params_[name]->to_string().c_str(), val->to_string().c_str());
		ABORT("");
	}

	params_[name]=val;
}

void Context::_set_df(const Id &id, const ValuePtr &val)
{
	if (dfs_.find(id)!=dfs_.end()) {
		fprintf(stderr, "Context::_set_df: df %s redefinition attempt "
			"from %s to %s\n", id.to_string().c_str(),
			dfs_[id]->to_string().c_str(), val->to_string().c_str());
		ABORT("");
	}

	dfs_[id]=val;
}

Id Context::_get_name(const Name &name) const
{
	auto it=names_.find(name);
	if (it==names_.end()) {
		fprintf(stderr, "Context::_get_name: not found: %s\n",
			name.c_str());
		ABORT("");
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
		ABORT("");
	} else {
		return it->second;
	}
}

ValuePtr Context::_get_df(const Id &id) const
{
	auto it=dfs_.find(id);
	if (it==dfs_.end()) {
		fprintf(stderr, "Context::_get_df: not found: %s in %s\n",
			id.to_string().c_str(), _to_string().c_str());
		ABORT("");
	} else {
		return it->second;
	}
}

bool Context::_can_eval(const json &expr) const
{
	if (expr["type"]=="iconst" || expr["type"]=="rconst"
			|| expr["type"]=="sconst") {
		return true;
	} else if (expr["type"]=="id") {
		if (!_can_eval_ref(expr["ref"])) {
			return false;
		} else {
			// param: 
			Name base_name=expr["ref"][0].get<std::string>();

			auto it=params_.find(base_name);

			if (it!=params_.end()) {
				auto val=it->second;
				if (val->type()!=Reference) {
					return true; // it's a constant
				} else {
					Id id=(*val);
					for (auto i=1u; i<expr["ref"].size(); i++) {
						id.push_back((int)(*_eval(expr["ref"][i])));
					}
					return dfs_.find(id)!=dfs_.end();
				}
			} else {
				return dfs_.find(_eval_ref(expr["ref"]))!=dfs_.end();
			}
		}
	} else if (expr["type"]=="+"
			|| expr["type"]=="/"
			|| expr["type"]=="-"
			|| expr["type"]=="*"
			|| expr["type"]=="<"
			|| expr["type"]==">"
			|| expr["type"]=="=="
			|| expr["type"]=="&&"
			|| expr["type"]=="%"
			) {
		for (auto op : expr["operands"]) {
			if (!_can_eval(op)) {
				return false;
			}
		}
		return true;
	} else if (expr["type"]=="icast") {
		return _can_eval(expr["expr"]);
	} else {
		fprintf(stderr, "Context::_can_eval: %s\n", expr.dump(2).c_str());
		ABORT("Not implemented type: " + expr["type"].dump());
		NIMPL
	}
}

bool Context::_can_eval_ref(const json &ref) const
{
	for (auto i=1u; i<ref.size(); i++) {
		if (!_can_eval(ref[i])) {
			return false;
		}
	}
	return true;
}

ValuePtr Context::_eval(const json &expr) const
{
	if (expr["type"]=="iconst") {
		return ValuePtr(new IntValue(expr["value"].get<int>()));
	} else if (expr["type"]=="rconst") {
		return ValuePtr(new RealValue(expr["value"].get<double>()));
	} else if (expr["type"]=="sconst") {
		return ValuePtr(new StringValue(expr["value"].get<std::string>()));
	} else if (expr["type"]=="id") {
		auto base_name=expr["ref"][0];
		auto it=params_.find(base_name);
		if (it!=params_.end()) {
			auto val=it->second;
			if (val->type()!=Reference) {
				if (expr["ref"].size()==1) {
					return val;
				} else {
					fprintf(stderr, "Context::_eval: index in parameter: "
						"%s\n", expr["ref"].dump(2).c_str());
					ABORT("");
				}
			} else {
				Id id=(*val);
				for (auto i=1u; i<expr["ref"].size(); i++) {
					id.push_back((int)(*_eval(expr["ref"][i])));
				}
				return _get_df(id);
			}
		} else {
			return _get_df(_eval_ref(expr["ref"]));
		}
	} else if (expr["type"]=="+"
			|| expr["type"]=="/"
			|| expr["type"]=="-"
			|| expr["type"]=="*"
			|| expr["type"]=="<"
			|| expr["type"]==">"
			|| expr["type"]=="=="
			|| expr["type"]=="&&"
			|| expr["type"]=="%"
			) {
		return _eval_op(expr["type"], expr["operands"]);
	} else if (expr["type"]=="icast") {
		return cast("int", _eval(expr["expr"]));
	} else {
		fprintf(stderr, "Context::_eval: %s\n", expr.dump(2).c_str());
		ABORT("Expr type not supported: " + expr["type"].dump());
	}
}

ValuePtr Context::_eval_op(const std::string &op, const json &ops) const
{
	if (ops.size()!=2) {
		fprintf(stderr, "Context::_eval_op: non-binary op: %s on %s\n",
			op.c_str(), ops.dump(2).c_str());
		NIMPL
	}
	
	ValuePtr op0=_eval(ops[0]), op1=_eval(ops[1]);

	if (op0->type()==Integer && op1->type()==Integer) {
		int res;
		if (op=="+") { res=(int)(*op0)+(int)(*op1); }
		else if (op=="/") { res=(int)(*op0)/(int)(*op1); }
		else if (op=="-") { res=(int)(*op0)-(int)(*op1); }
		else if (op=="*") { res=(int)(*op0)*(int)(*op1); }
		else if (op=="%") { res=(int)(*op0)%(int)(*op1); }
		else if (op=="<") { res=((int)(*op0)<(int)(*op1)? 1: 0); }
		else if (op=="==") { res=((int)(*op0)==(int)(*op1)? 1: 0); }
		else if (op=="&&") { res=((int)(*op0)&&(int)(*op1)? 1: 0); }
		else { 
			ABORT("Can't eval: " + op0->to_string() + op 
			+ op1->to_string());
		}
		return ValuePtr(new IntValue(res));
	} else if ((op0->type()==Integer||op0->type()==Real)
			&& ((op1->type()==Integer||op1->type()==Real))) {
		double res;
		if (op=="+") { res=(double)(*op0)+(double)(*op1); }
		else if (op=="/") { res=(double)(*op0)/(double)(*op1); }
		else if (op=="-") { res=(double)(*op0)-(double)(*op1); }
		else if (op=="*") { res=(double)(*op0)*(double)(*op1); }
		else if (op=="<") { return ValuePtr(new IntValue(
			(double)(*op0)<(double)(*op1)? 1: 0)); }
		else if (op==">") { return ValuePtr(new IntValue(
			(double)(*op0)>(double)(*op1)? 1: 0)); }
		else if (op=="==") { return ValuePtr(new IntValue(
			(double)(*op0)==(double)(*op1)? 1: 0)); }
		else { fprintf(stderr, "OP NIMPL: %s\n", op.c_str()); NIMPL }
		return ValuePtr(new RealValue(res));
	} else {
		fprintf(stderr, "Operation result type not deduced for %s %s %s\n",
			op0->to_string().c_str(), op.c_str(), op1->to_string().c_str());
		NIMPL
	}
}

Id Context::_eval_ref(const json &ref) const
{
	Name ref_name=ref[0].get<std::string>();

	Id id;

	auto itp=params_.find(ref_name);

	if (itp!=params_.end()) {
		assert(names_.find(ref_name)==names_.end()); // can't be both in
			// names_ and params
		if (itp->second->type()==Reference) {
			id=*itp->second;
		} else {
			fprintf(stderr, "Context::_eval_ref: name %s is present"
				" in params, but is not a name: %s\n",
				ref_name.c_str(), itp->second->to_string().c_str());
			ABORT("");
		}
	} else {
		auto it=names_.find(ref_name);

		if (it==names_.end()) {
			fprintf(stderr, "Context::_eval_ref: name %s is missing"
				" in ref %s\n",
				ref_name.c_str(), ref.dump().c_str());
			ABORT("");
		}
		id=it->second;
	}

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
		fprintf(stderr, "Context::_pull_name: name %s is already set "
			"to %s, but got %s from the other context\n",
				name.c_str(), names_[name].to_string().c_str(),
				id.to_string().c_str());
		ABORT("");
	}
}

void Context::_pull_names(const Context &ctx)
{
	for (auto el : ctx.names_) {
		_pull_name(el.first, ctx);
	}
}

void Context::_pull_param(const Name &name, const Context &ctx)
{
	auto val=ctx.get_param(name);
	auto it=params_.find(name);
	if (it==params_.end()) {
		_set_param(name, val);
	} else {
		fprintf(stderr, "Context::_pull_param: name %s is already set "
			"to %s, but got %s from the other context\n",
				name.c_str(), params_[name]->to_string().c_str(),
				val->to_string().c_str());
		ABORT("");
	}
}

void Context::_pull_params(const Context &ctx)
{
	for (auto el : ctx.params_) {
		_pull_param(el.first, ctx);
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

std::string Context::to_string() const
{
	std::lock_guard<std::mutex> lk(m_);
	return _to_string();
}

std::string Context::_to_string() const
{
	std::ostringstream os;

	os << "{";
	for (auto name : names_) {
		os << " " << name.second;
	}
	for (auto param : params_) {
		os << " " << param.first << ":" << (*param.second);
	}
	for (auto df : dfs_) {
		os << " " << df.first << "=" << (*df.second);
	}
	os << " }";
	return os.str();
}

ValuePtr Context::cast(const std::string &type, const ValuePtr &val)
{
	if (type=="int") {
		return ValuePtr(new IntValue((int)(*val)));
	} else if (type=="string") {
		return ValuePtr(new StringValue((std::string)(*val)));
	} else if (type=="name") {
		return ValuePtr(new NameValue((Id)(*val)));
	} else if (type=="value") {
		return ValuePtr(new CustomValue(*val));
	} else if (type=="real") {
		return ValuePtr(new RealValue(*val));
	} else {
		fprintf(stderr, "Context::cast: casting (%s)%s\n",
			type.c_str(), val->to_string().c_str());
		NIMPL
	}
}

std::set<std::pair<Name, std::vector<int> > > Context::get_names(
	const Id &id) const
{
	std::set<std::pair<Name, std::vector<int> > > res;
	// process names_
	for (auto it : names_) {
		auto name=it.first;
		auto id_prefix=it.second;
		printf("MATCH %s ?= %s: %s\n",
			id.to_string().c_str(),
			id_prefix.to_string().c_str(), (id.starts_with(id_prefix)?
				"Yes": "No"));
		if (id.starts_with(id_prefix)) {
			res.insert(std::make_pair(name, std::vector<int>(
				id.begin()+id_prefix.size(), id.end())));
		}
	}
	// process params
	for (auto it : params_) {
		if (it.second->type()!=Reference) {
			continue;
		}
		auto name=it.first;
		auto id_prefix=(Id)(*it.second);
		printf("P_MATCH %s ?= %s: %s\n",
			id.to_string().c_str(),
			id_prefix.to_string().c_str(), (id.starts_with(id_prefix)?
				"Yes": "No"));
		if (id.starts_with(id_prefix)) {
			WARN("result: " + name);
			res.insert(std::make_pair(name, std::vector<int>(
				id.begin()+id_prefix.size(), id.end())));
		}
	}
	return res;
}
