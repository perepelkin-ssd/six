#include "df_pusher.h"

DfPusher::DfPusher(ThreadPool *pool, std::function<void(int)>wlc)
	: pool_(pool), wl_changer_(wlc)
{}

void DfPusher::push(const Id &cfid, const Id &dfid, const ValuePtr &val)
{
	printf("\tDFPUSHED %s:\n", cfid.to_string().c_str());
	std::lock_guard<std::mutex> lk(m_);
	wl_changer_(1);

	Port &port=ports_[cfid]; // will be created if none

	if (port.cb_) {
		printf("\tDFPUSHED: already opened\n");
		// already opened
		assert(port.queue_.empty());
		auto cb=port.cb_;
		auto wlc=wl_changer_;
		pool_->submit([wlc, cb, dfid, val](){
			cb(dfid, val);
			wlc(-1);
		});
	} else {
		printf("\tDFPUSHED: not yet opened\n");
		// not yet opened
		port.queue_.push(std::make_pair(dfid, val));
	}
}

void DfPusher::open(const Id &cfid, Callback cb)
{
	std::lock_guard<std::mutex> lk(m_);
	wl_changer_(1);

	Port &port=ports_[cfid]; // will be created if none

	if (port.cb_) {
		throw std::runtime_error(std::string("port already opened: ")
			+ cfid.to_string());
	}

	port.cb_=cb;

	while (!port.queue_.empty()) {
		auto item=port.queue_.front();
		port.queue_.pop();

		auto wlc=wl_changer_;
		pool_->submit([wlc, cb, item](){
			cb(item.first, item.second);
			wlc(-1);
		});
	}
}

void DfPusher::close(const Id &cfid)
{
	std::lock_guard<std::mutex> lk(m_);

	auto it=ports_.find(cfid);

	if (it==ports_.end()) {
		throw std::runtime_error(std::string("port not open: ")
			+ cfid.to_string());
	}

	assert(it->second.cb_);
	assert(it->second.queue_.empty());

	ports_.erase(it);
	wl_changer_(-1);
}

