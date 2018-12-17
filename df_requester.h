#pragma once

#include <map>
#include <set>

#include "id.h"
#include "printable.h"
#include "thread_pool.h"
#include "value.h"

// DataFragment Requester model
// After DF is produced it is stored on the node until explicit deletion
// (put and del methods).
// The DF can be requested from the node (request), which will cause
// DF to be called-back.
// If the request is made before the DF is stored, the request awaits
// until DF will appear, and will be called-back then
//
// All callbacks are done via the thread pool. Workload change notification
// is also supported.
class DfRequester : public Printable
{
public:
	typedef std::function<void (const ValuePtr &)> Callback;
	typedef std::function<void ()> RequestCb;

	DfRequester(ThreadPool *, std::function<void(int)> workload_changer);

	void request(const Id &, Callback);

	// Each request causes RequestCb invocation
	void put(const Id &, const ValuePtr &, RequestCb cb=nullptr);
	void del(const Id &);

	virtual std::string to_string() const;
private:
	// All callbacks are invoked via external pool
	ThreadPool *pool_;
	mutable std::mutex m_;
	
	// dfs are stored infinitely (until explicit deletion), requests
	// wait for dfs to appear
	std::map<Id, std::pair<ValuePtr, RequestCb> > dfs_;
	std::map<Id, std::queue<Callback> > requests_;
	std::set<Id> dels_;

	std::function<void(int)> wl_changer_;
};
