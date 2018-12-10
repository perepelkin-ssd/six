#include "df_requester.h"

DfRequester::DfRequester(ThreadPool *pool, std::function<void(int)> wlc)
	: pool_(pool), wl_changer_(wlc)
{}

void DfRequester::request(const Id &id, DfRequester::Callback cb)
{
	std::lock_guard<std::mutex> lk(m_);
	wl_changer_(1);

	auto it=dfs_.find(id);

	if (dels_.find(id)!=dels_.end()) {
		throw std::runtime_error(std::string("request df after del: ")
			+ id.to_string());
	}

	if (it!=dfs_.end()) {
		ValuePtr val=it->second.first;
		RequestCb rcb=it->second.second;

		auto wlc=wl_changer_;
		pool_->submit([wlc, cb, val, rcb]() {
			cb(val);
			if (rcb) {
				rcb();
			}
			wlc(-1);
		});
	} else {
		requests_[id].push(cb);
	}
}

void DfRequester::put(const Id &id, const ValuePtr &val, RequestCb rcb)
{
	std::lock_guard<std::mutex> lk(m_);
	wl_changer_(1);
	
	auto it=dfs_.find(id);
	
	if (it!=dfs_.end() && it->second.first) {
		throw std::runtime_error(
			std::string("DF exists: " + id.to_string()));
	}

	auto it3=dels_.find(id);

	if (it3!=dels_.end()) {
		assert(requests_.find(id)==requests_.end());

		dels_.erase(it3);
		wl_changer_(-2);
		return;
	}

	dfs_[id]=std::make_pair(val, rcb);


	auto it2=requests_.find(id);

	if (it2!=requests_.end()) {
		while (!it2->second.empty()) {
			auto cb=it2->second.front();
			it2->second.pop();

			auto wlc=wl_changer_;
			pool_->submit([wlc, cb, val, rcb]() {
				cb(val);
				if (rcb) {
					rcb();
				}
				wlc(-1);
			});
		}
		requests_.erase(it2);
	}
}

void DfRequester::del(const Id &id)
{
	std::lock_guard<std::mutex> lk(m_);

	//wl_changer_(-1);

	auto it=dfs_.find(id);

	if (it==dfs_.end()) {
		//throw std::runtime_error(std::string("Cannot delete: no such df: ")
		//	+ id.to_string());
		if (dels_.find(id)!=dels_.end()) {
			throw std::runtime_error(std::string(
			"Duplicate df del request: ") + id.to_string());
		}

		if (requests_.find(id)!=requests_.end()) {
			throw std::runtime_error(std::string(
				"DF del, but requests present for df: ") + id.to_string());
		}

		dels_.insert(id);
		wl_changer_(1);
		return;
	}

	dfs_.erase(it);
	wl_changer_(-1);

	if (requests_.find(id)!=requests_.end()) {
		throw std::runtime_error(std::string(
			"requests present at df removal: " + id.to_string()));
	}
}

