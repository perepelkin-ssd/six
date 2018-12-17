#include "df_pusher.h"

#include <sstream>

DfPusher::DfPusher(ThreadPool *pool, std::function<void(int)>wlc)
	: pool_(pool), wl_changer_(wlc)
{}

void DfPusher::push(const Id &cfid, const Id &dfid, const ValuePtr &val)
{
	std::lock_guard<std::mutex> lk(m_);
	wl_changer_(1);

	Port &port=ports_[cfid]; // will be created if none

	if (port.cb_) {
		// already opened
		assert(port.queue_.empty());
		auto cb=port.cb_;
		auto wlc=wl_changer_;
		pool_->submit([wlc, cb, dfid, val](){
			cb(dfid, val);
			wlc(-1);
		});
	} else {
		// not yet opened
		port.queue_.push_back(std::make_pair(dfid, val));
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
		port.queue_.pop_front();

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

std::string DfPusher::to_string() const
{
	std::lock_guard<std::mutex> lk(m_);

	std::ostringstream os;

	for (auto p : ports_) {
		os << p.first << "(";
		bool need_comma=false;
		for (auto i : p.second.queue_) {
			if (need_comma) {
				os << ", ";
			} else {
				need_comma=true;
			}
			os << i.first;
		}
		os << ")";
	}
	return os.str();
}
