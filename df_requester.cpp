#include "df_requester.h"

DfRequester::DfRequester(ThreadPool *pool, std::function<void(int)> wlc)
	: pool_(pool), wl_changer_(wlc)
{}

void DfRequester::request(const Id &id, DfRequester::Callback cb)
{
	std::lock_guard<std::mutex> lk(m_);
	wl_changer_(1);

	auto it=dfs_.find(id);

	if (it!=dfs_.end()) {
		ValuePtr val=it->second;

		auto wlc=wl_changer_;
		pool_->submit([wlc, cb, val]() {
			cb(val);
			wlc(-1);
		});
	} else {
		requests_[id].push(cb);
	}
}

void DfRequester::put(const Id &id, const ValuePtr &val)
{
	std::lock_guard<std::mutex> lk(m_);
	wl_changer_(1);
	
	auto it=dfs_.find(id);
	
	if (it!=dfs_.end() && it->second) {
		throw std::runtime_error(
			std::string("DF exists: " + id.to_string()));
	}

	dfs_[id]=val;

	auto it2=requests_.find(id);

	if (it2!=requests_.end()) {
		while (!it2->second.empty()) {
			auto cb=it2->second.front();
			it2->second.pop();

			auto wlc=wl_changer_;
			pool_->submit([wlc, cb, val]() {
				cb(val);
				wlc(-1);
			});
		}
		requests_.erase(it2);
	}
}

void DfRequester::del(const Id &id)
{
	std::lock_guard<std::mutex> lk(m_);

	wl_changer_(-1);

	auto it=dfs_.find(id);

	if (it==dfs_.end()) {
		throw std::runtime_error(std::string("Cannot delete: no such df: ")
			+ id.to_string());
	}

	dfs_.erase(it);

	if (requests_.find(id)!=requests_.end()) {
		throw std::runtime_error(std::string(
			"requests present at df removal: " + id.to_string()));
	}
}

