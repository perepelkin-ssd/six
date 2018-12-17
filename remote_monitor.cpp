#include "remote_monitor.h"

#include "common.h"

RemoteMonitor::RemoteMonitor(){}

BufHandler *RemoteMonitor::create(std::function<bool (BufferPtr &)> handler)
{
	RemoteMonitorPtr mon(new RemoteMonitor());
	mon->handler_=handler;
	mon->self_=mon;
	return dynamic_cast<BufHandler *>(mon.get());
}

void RemoteMonitor::handle(BufferPtr &buf)
{
	auto self=self_; // to prevent self destruction immediately 
	// after self_.reset()
	NOTE("RemoteMonitor: start handle");
	{
		std::lock_guard<std::mutex> lk(m_);

		if (!self_) {
			throw std::runtime_error("RemoteMonitor::handle: handle "
				"after stop");
		}

		auto handler=handler_;
	}

	NOTE("RemoteMonitor: user handle start");
	bool keep_alive=handler_(buf);
	NOTE("RemoteMonitor: user handle finish");

	if (!keep_alive) {
		std::lock_guard<std::mutex> lk(m_);
		
		if (!self_) {
			throw std::runtime_error("RemoteMonitor::handle: duplicate "
				"stop");
		}
		self_.reset(); // destroy pointer to self
		handler_=nullptr; // destroy closure
	}
	NOTE("RemoteMonitor: stop handle");
}

BufHandler *create_counter(size_t count, std::function<void()> cb)
{
	std::shared_ptr<size_t> counter(new size_t(count));
	std::shared_ptr<std::mutex> m(new std::mutex());

	return RemoteMonitor::create([counter, m, cb](BufferPtr &){
		bool stop_flag;
		{
			std::lock_guard<std::mutex> lk(*m);

			assert(*counter>0);

			(*counter)--;

			stop_flag=(*counter==0);
		}

		if (stop_flag) {
			cb();
		}

		return !stop_flag;
	});
}

BufHandler *remote_callback(std::function<void(BufferPtr &)> cb)
{
	return RemoteMonitor::create([cb](BufferPtr &buf) {
		cb(buf);
		return false;
	});

}
