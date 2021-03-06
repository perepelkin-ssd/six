#pragma once

#include <set>

#include "context.h"
#include "factory.h"
#include "json.h"
#include "locator.h"
#include "task.h"

class JfpExec : public Task
{
public:
	virtual ~JfpExec() {}

	JfpExec(const Id &fp_id, const Id &cf_id, const std::string &jdump,
		const LocatorPtr &, Factory &);
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
	LocatorPtr loc_;
	std::map<Id, json> locators_;
	// e.g.: "x": {
	//	"ref": ["x", {..i..}],
	//	"expr": {..i..}
	//	"type": "locator_cyclic"
	// }
	std::mutex m_;
	bool executed_flag_;

	const json &fp() { return *fp_; }

	void resolve_args(const EnvironPtr &);

	void init_child_context(JfpExec *child, const Context &base);
	void init_child_context_arg(JfpExec *child,
		const json &, const Context &base);

	void check_exec(const EnvironPtr &);
	void exec(const EnvironPtr &);
	void exec_exec(const EnvironPtr &);
	void exec_for(const EnvironPtr &);
	void exec_while(const EnvironPtr &);
	void exec_if(const EnvironPtr &);
	void exec_struct(const EnvironPtr &, const json &);
	void exec_extern(const EnvironPtr &, const Name &);
	void spawn_body(const EnvironPtr &, const json &, const Context &);

	void df_computed(const EnvironPtr &env, const Id &,
		const ValuePtr &);

	void request_requested_dfs(const EnvironPtr &env);

	void do_afterwork(const EnvironPtr &, const Context &);

	void _assert_rules();

	LocatorPtr get_global_locator(const Id &, const EnvironPtr &,
		const Context &);
	NodeId glocate_next_node(const Id &, const EnvironPtr &,
		const Context &);
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
