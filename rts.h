#ifndef RTS_H_
#define RTS_H_

#include <condition_variable>
#include <map>
#include <mutex>

#include "cf_env.h"
#include "comm.h"
#include "context.h"
#include "fp.h"
#include "id.h"
#include "idle_stopper.h"
#include "rts_env.h"

class RTS
{
public:
	~RTS();
	RTS(Comm &);

	CfEnv &get_env();

	// Root node is the first node to wait. 
	void wait_all(bool is_root_node);
private:
	std::mutex m_;
	std::condition_variable cv_;
	Comm &comm_;
	RtsEnv env_;
	IdleStopper<NodeId> *stopper_;
	bool idle_flag_;

	friend class RtsEnv;

	enum TAGS {
		TAG_CF,
		TAG_DF,
		TAG_DF_VALUE,
		TAG_IDLE_STOPPER,
		TAG_IDLE
	};

	void on_message(const NodeId &src, const Tag &tag, const void *buf,
		size_t size);
};

#endif // RTS_H_
