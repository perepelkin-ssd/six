#pragma once

#include <map>
#include <mutex>

#include "factory.h"
#include "id.h"
#include "serializable.h"
#include "value.h"

class Context : public Serializable
{
public:
	Context() {}
	Context(BufferPtr &buf, Factory &fact);

	Context &operator=(const Context &);

	void set_name(const Name &, const Id &);
	void set_param(const Name &, const ValuePtr &);
	void set_df(const Id &, const ValuePtr &);

	bool has_name(const Name &) const;
	bool has_param(const Name &) const;
	bool has_df(const Id &) const;

	Id get_name(const Name &) const;
	ValuePtr get_param(const Name &) const;
	ValuePtr get_df(const Id &) const;

	Id eval_ref(const json &) const;
	ValuePtr eval(const json &expr) const;

	void pull_name(const Name &name, const Context &);

	virtual void serialize(Buffers &) const;
private:
	mutable std::mutex m_;
	std::map<Name, Id> names_;
	std::map<Name, ValuePtr> params_;
	std::map<Id, ValuePtr> dfs_;

	void _set_name(const Name &, const Id &);
	void _set_param(const Name &, const ValuePtr &);
	void _set_df(const Id &, const ValuePtr &);

	Id _get_name(const Name &) const;
	ValuePtr _get_param(const Name &) const;
	ValuePtr _get_df(const Id &) const;

	void _pull_name(const Name &name, const Context &);

	Id _eval_ref(const json &) const;
	ValuePtr _eval(const json &expr) const;
};
