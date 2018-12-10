#pragma once

#include <set>

#include "json.hpp"

#include "factory.h"
#include "task.h"

class JfpExec : public Task, public BufHandler
{
public:
	virtual ~JfpExec() {}

	JfpExec(const Id &fp_id, const Id &cf_id, const std::string &jdump,
		Factory &);
	JfpExec(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &bufs) const;

	virtual std::string to_string() const;

	virtual void handle(BufferPtr &);
private:
	Factory &fact_;
	Id fp_id_, cf_id_;
	nlohmann::json j_;
	const nlohmann::json *fp_;
	std::map<Name, Id> names_;
	std::map<Name, ValuePtr> params_;
	std::map<Id, ValuePtr> dfs_;
	std::set<std::pair<Id, Id> > dfpush_simple_noidx_;
	std::map<Id, size_t> dfreqcount_simple_noidx_;
	bool pushed_flag_;
	std::map<Id, std::function<void(BufferPtr &)> > rcbs_;
	EnvironPtr env_holder_;

	const nlohmann::json &fp() { return *fp_; }

	void resolve_args(const EnvironPtr &);
	std::set<Id> get_deps();
	void extract_deps(std::set<Id> &deps, const nlohmann::json &expr);
	void extract_name_deps(std::set<Id> &deps, const nlohmann::json &expr);
	void extract_deps_idexpr(std::set<Id> &deps,
		const nlohmann::json &idexpr);
	Id eval_ref(const nlohmann::json &ref);

	void init_child_context(JfpExec *child);
	void init_child_context_arg(JfpExec *child,
		const nlohmann::json &);

	ValuePtr eval(const nlohmann::json &expr);

	NodeId get_next_node(const EnvironPtr &, const Id &);

	void check_exec(const EnvironPtr &);
	void exec(const EnvironPtr &);
	void exec_struct(const EnvironPtr &, const nlohmann::json &);
	void exec_extern(const EnvironPtr &, const Name &);

	void df_computed(const EnvironPtr &env, const nlohmann::json &ref,
		const ValuePtr &);

	RPtr remote_callback(const EnvironPtr &, const Id &rcbid,
		std::function<void (BufferPtr &)>);
};

class JfpReg : public Task
{
public:
	virtual ~JfpReg() {}

	JfpReg(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

private:
	Id fp_id_;
	nlohmann::json j_;
	RPtr rptr_;
};
