#pragma once

#include <functional>
#include <map>

#include "id.h"
#include "printable.h"
#include "thread_pool.h"
#include "value.h"

// DataFragment Pusher model:
//
// Once DF is produced, it is sent to its consumers via "push" (id+val)
//
// Note: delivery must be done elsewhere, local node is the place to
// where DFs are pushed, and where they are got from
//
// Consumer "open"s a port causing all already pushed DFs to be
// called back. Further pushes cause callbacks.
//
// After all DFs are received, the port must be "closed".
//
// All callbacks are sent via thread pool, not directly from calls.
// DfPusher also notifies on workload change (abstract size_t counter).
class DfPusher : public Printable
{
public:
	typedef std::function<void (const Id &dfid, const ValuePtr &val)>
		Callback;

	DfPusher(ThreadPool *,
		std::function <void (int)>workload_changer);

	void push(const Id &cfid, const Id &dfid, const ValuePtr &val);
	void open(const Id &cfid, Callback);
	void close(const Id &cfid);

	virtual std::string to_string() const;
private:
	struct Port
	{
		std::deque<std::pair<Id, ValuePtr> > queue_;
		Callback cb_;

		Port() : cb_(nullptr) {}
	};
	mutable std::mutex m_;
	ThreadPool *pool_;
	std::map<Id, Port> ports_;
	std::function<void(int)> wl_changer_;
};
