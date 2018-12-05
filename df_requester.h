#pragma once

#include <map>
#include <set>

#include "id.h"
#include "thread_pool.h"
#include "value.h"

class DfRequester
{
public:
	typedef std::function<void (const ValuePtr &)> Callback;

	DfRequester(ThreadPool *, std::function<void(int)> workload_changer);

	void request(const Id &, Callback);
	void put(const Id &, const ValuePtr &);
	void del(const Id &);
private:
	// All callbacks are invoked via external pool
	ThreadPool *pool_;
	std::mutex m_;
	
	// dfs are stored infinitely (until explicit deletion), requests
	// wait for dfs to appear
	std::map<Id, ValuePtr> dfs_;
	std::map<Id, std::queue<Callback> > requests_;
	std::set<Id> dels_;

	std::function<void(int)> wl_changer_;
};
