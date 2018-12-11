#pragma once

#include <mutex>

#include "buf_handler.h"

class RemoteMonitor;

typedef std::shared_ptr<RemoteMonitor> RemoteMonitorPtr;

class RemoteMonitor : public BufHandler
{
	RemoteMonitor();
public:
	virtual ~RemoteMonitor() {}

	// handler returns true to keep alive and false to stop
	static BufHandler *create(std::function<bool (BufferPtr &)> handler);

	virtual void handle(BufferPtr &);

private:
	std::mutex m_;
	RemoteMonitorPtr self_;
	std::function<bool (BufferPtr &)> handler_;
};

// Create a counter remote monitor, which invokes cb after count invocations
BufHandler *create_counter(size_t count, std::function<void()> cb);

BufHandler *remote_callback(std::function<void(BufferPtr &)> cb);
