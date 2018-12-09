#pragma once

#include <set>

#include "json.hpp"

#include "factory.h"
#include "task.h"

class JfpExec : public Task
{
public:
	virtual ~JfpExec() {}

	JfpExec(const Id &fp_id, const Id &cf_id, const std::string &jdump);
	JfpExec(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &bufs) const;

	virtual std::string to_string() const;
private:
	Id fp_id_, cf_id_;
	nlohmann::json j_;
	const nlohmann::json *fp_;
	std::map<Name, Id> names_;
	std::map<Name, ValuePtr> params_;
	std::map<Id, ValuePtr> dfs_;

	const nlohmann::json &fp() { return *fp_; }

	void resolve_args(const EnvironPtr &);
	std::set<Id> get_args();
	std::set<Id> get_args_exec();

	void init_child_context(JfpExec *child);

	NodeId get_next_node(const EnvironPtr &);

	void exec(const EnvironPtr &);
	void exec_struct(const EnvironPtr &, const nlohmann::json &);
	void exec_extern(const EnvironPtr &, const nlohmann::json &);
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
