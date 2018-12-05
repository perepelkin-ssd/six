#ifndef RTS_H_
#define RTS_H_

#include <condition_variable>
#include <map>
#include <mutex>

#include "comm.h"
#include "context.h"
#include "df_pusher.h"
#include "df_requester.h"
#include "factory.h"
#include "fp.h"
#include "id.h"
#include "idle_stopper.h"
#include "task.h"
#include "thread_pool.h"

enum TAGS {
	TAG_TASK,
	TAG_IDLE_STOPPER,
	TAG_IDLE
};

class RTS
{
public:
	~RTS();
	RTS(Comm &);

	// Root node is the first node to wait. 
	void wait_all(bool is_root_node);

	void submit(const TaskPtr &);

	Factory &factory() { return factory_; }

	void change_workload(int delta);

	Id create_id(const Name &label="", const std::vector<int> &idx={});

	DfPusher &df_pusher() { return df_pusher_; }
	DfRequester &df_requester() { return df_requester_; }
private:
	std::mutex m_;
	std::condition_variable cv_;
	Comm &comm_;
	IdleStopper<NodeId> *stopper_;
	bool idle_flag_;
	Factory factory_;
	ThreadPool pool_;
	size_t workload_;
	int next_id_;
	DfPusher df_pusher_;
	DfRequester df_requester_;

	void on_message(const NodeId &src, BufferPtr);
};

#endif // RTS_H_
