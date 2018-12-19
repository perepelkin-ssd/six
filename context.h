#pragma once

#include <map>
#include <mutex>
#include <set>

#include "factory.h"
#include "id.h"
#include "serializable.h"
#include "value.h"

class Context : public Serializable, public Printable
{
public:
	Context() {}
	Context(const Context &);
	Context(BufferPtr &buf, Factory &fact);

	Context &operator=(const Context &);

	void set_name(const Name &, const Id &);
	void set_param(const Name &, const ValuePtr &,
		bool allow_override=false);
	void set_df(const Id &, const ValuePtr &);

	bool has_name(const Name &) const;
	bool has_param(const Name &) const;
	bool has_df(const Id &) const;

	Id get_name(const Name &) const;
	ValuePtr get_param(const Name &) const;
	ValuePtr get_df(const Id &) const;

	bool can_eval(const json &) const;
	bool can_eval_ref(const json &) const;

	ValuePtr eval(const json &) const;
	Id eval_ref(const json &) const;

	void pull_name(const Name &name, const Context &);
	void pull_names(const Context &);
	void pull_params(const Context &);

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;
	
	static ValuePtr cast(const std::string &type, const ValuePtr &val);

	// get all base names, through which given ID is accessible
	std::set<std::pair<Name, std::vector<int> > > 
		get_names(const Id &) const;
private:
	mutable std::mutex m_;
	std::map<Name, Id> names_;
	std::map<Name, ValuePtr> params_;
	std::map<Id, ValuePtr> dfs_;

	void _set_name(const Name &, const Id &);
	void _set_param(const Name &, const ValuePtr &,
		bool allow_override=false);
	void _set_df(const Id &, const ValuePtr &);

	Id _get_name(const Name &) const;
	ValuePtr _get_param(const Name &) const;
	ValuePtr _get_df(const Id &) const;

	bool _can_eval(const json &) const;
	bool _can_eval_ref(const json &) const;

	ValuePtr _eval(const json &expr) const;
	ValuePtr _eval_op(const std::string &op, const json &expr) const;
	Id _eval_ref(const json &) const;

	void _pull_name(const Name &name, const Context &);
	void _pull_names(const Context &);
	void _pull_param(const Name &name, const Context &);
	void _pull_params(const Context &);

	virtual std::string _to_string() const;
};
