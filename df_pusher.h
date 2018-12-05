#pragma once

#include <functional>
#include <map>

#include "id.h"
#include "thread_pool.h"
#include "value.h"

class DfPusher
{
public:
	typedef std::function<void (const Id &dfid, const ValuePtr &val)>
		Callback;

	DfPusher(ThreadPool *,
		std::function <void (int)>workload_changer);

	void push(const Id &cfid, const Id &dfid, const ValuePtr &val);
	void open(const Id &cfid, Callback);
	void close(const Id &cfid);

private:
	struct Port
	{
		std::queue<std::pair<Id, ValuePtr> > queue_;
		Callback cb_;

		Port() : cb_(nullptr) {}
	};
	std::mutex m_;
	ThreadPool *pool_;
	std::map<Id, Port> ports_;
	std::function<void(int)> wl_changer_;
};
