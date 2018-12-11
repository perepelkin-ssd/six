#pragma once

#include <set>

#include "context.h"
#include "factory.h"
#include "json.h"
#include "task.h"

class JfpExec : public Task
{
public:
	virtual ~JfpExec() {}

	JfpExec(const Id &fp_id, const Id &cf_id, const std::string &jdump,
		Factory &);
	JfpExec(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &bufs) const;

	virtual std::string to_string() const;
private:
	Factory &fact_;
	Id fp_id_, cf_id_;
	json j_;
	const json *fp_;
	Context ctx_;
	bool pushed_flag_;
	std::set<Id> requested_;

	const json &fp() { return *fp_; }

	void resolve_args(const EnvironPtr &);

	void init_child_context(JfpExec *child);
	void init_child_context_arg(JfpExec *child,
		const json &);

	NodeId get_next_node(const EnvironPtr &, const Id &);

	void check_exec(const EnvironPtr &);
	void exec(const EnvironPtr &);
	void exec_struct(const EnvironPtr &, const json &);
	void exec_extern(const EnvironPtr &, const Name &);

	void df_computed(const EnvironPtr &env, const json &ref,
		const ValuePtr &);

	void request_missing(const EnvironPtr &env);
};

class JfpReg : public Task
{
public:
	virtual ~JfpReg() {}

	JfpReg(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

private:
	Id fp_id_;
	json j_;
	RPtr rptr_;
};
