#ifndef RTS_H_
#define RTS_H_

#include <condition_variable>
#include <map>
#include <mutex>

#include "code_lib.h"
#include "comm.h"
#include "df_pusher.h"
#include "df_requester.h"
#include "factory.h"
#include "id.h"
#include "idle_stopper.h"
#include "task.h"
#include "thread_pool.h"

// Tag is sent as the head of the message
enum TAGS {
	TAG_TASK,
	TAG_IDLE_STOPPER,
	TAG_IDLE
};

class RTS : public Printable
{
public:
	~RTS();
	RTS(Comm &, CodeLib &);

	// Root node is the first node to wait (the node, which initializes
	// idleness check via idle_stopper). Make sure wait_all is called
	// on the root node only after initial workload is submitted.
	void wait_all(bool is_root_node);

	void submit(const TaskPtr &);

	Factory &factory() { return factory_; }

	// Workload tracker via abstract size_t counter. Non-zero value
	// means something has to be done before full stop.
	void change_workload(int delta);

	// Create unique id - (node, uid) pair with custom label and
	// supplementary indices. uid is node-wide unique id.
	Id create_id(const Name &label="", const std::vector<int> &idx={});

	DfPusher &df_pusher() { return df_pusher_; }
	DfRequester &df_requester() { return df_requester_; }

	ValuePtr get(const Id &key) const;
	ValuePtr set(const Id &key, const ValuePtr &val);
	ValuePtr del(const Id &key);

	virtual std::string to_string() const;

private:
	mutable std::mutex m_;
	std::condition_variable cv_;
	Comm &comm_;
	CodeLib &clib_;
	IdleStopper<NodeId> *stopper_;
	bool idle_flag_;
	Factory factory_;

	// The pool_ is used for some slave services, such as df_pusher
	ThreadPool pool_;
	size_t workload_;
	size_t wl_pusher_, wl_requester_;
	int next_id_;
	DfPusher df_pusher_;
	DfRequester df_requester_;
	std::map<Id, ValuePtr> vals_;

	void on_message(const NodeId &src, BufferPtr);

	void change_workload_pusher(int delta);
	void change_workload_requester(int delta);
};

#endif // RTS_H_
