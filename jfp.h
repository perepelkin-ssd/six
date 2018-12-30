#pragma once

#include <set>

#include "context.h"
#include "factory.h"
#include "locator.h"
#include "task.h"

extern "C" {
#include "fp.h"
}

class JfpExec : public Task
{
public:
	virtual ~JfpExec() {}

	JfpExec(const Id &fp_id, const Id &cf_id, WORD cf_ofs,
		const LocatorPtr &, Factory &);
	JfpExec(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &bufs) const;

	virtual std::string to_string() const;

	static void exec_main(const EnvironPtr &, const Id &fp_id, Factory &,
		const std::string main_arg="");
private:
	Factory &fact_;
	Id fp_id_, cf_id_;
	const void *cf_;
	WORD cf_ofs_;
	const void *fp_;
	Context ctx_;
	bool pushed_flag_;
	std::set<Id> requested_;
	LocatorPtr loc_;
	std::map<Id, WORD> locators_;
	bool wstart_is_overriden_;
	int wstart_override_;
	// e.g.: "x": {
	//	"ref": ["x", {..i..}],
	//	"expr": {..i..}
	//	"type": "locator_cyclic"
	// }
	std::mutex m_;
	bool executed_flag_;

	void resolve_args(const EnvironPtr &);

	static void init_child_context(JfpExec *child, const Context &base,
		const std::map<Id, WORD> &);
	static void init_child_context_arg(JfpExec *child,
		const void *, const Context &base);

	void check_exec(const EnvironPtr &);
	void exec(const EnvironPtr &);
	void exec_exec(const EnvironPtr &);
	void exec_for(const EnvironPtr &);
	void exec_while(const EnvironPtr &);
	void exec_if(const EnvironPtr &);
	void exec_struct(const EnvironPtr &, const void *);
	void exec_extern(const EnvironPtr &, const void *sub);
	static void spawn_body(const EnvironPtr &, const void *body,
		const void *rules, const Context &, const LocatorPtr &,
		const std::map<Id, WORD> &locators, const void *fp,
		const Id &fp_id, Factory &);

	void df_computed(const EnvironPtr &env, const Id &,
		const ValuePtr &);

	void request_requested_dfs(const EnvironPtr &env);

	void do_afterwork(const EnvironPtr &, const Context &);

	void _assert_rules();

	static LocatorPtr get_global_locator(const Id &, const EnvironPtr &,
		const Context &, const std::map<Id, WORD> &locators,
		const void *fp);
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
	BufferPtr fp_buf_;
	RPtr rptr_;
};
