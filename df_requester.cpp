#include "df_requester.h"

#include <sstream>

DfRequester::DfRequester(ThreadPool *pool, std::function<void(int)> wlc)
	: pool_(pool), wl_changer_(wlc)
{}

void DfRequester::request(const Id &id, DfRequester::Callback cb)
{
	std::lock_guard<std::mutex> lk(m_);
	NOTE("DfRequester::request " + id.to_string());
	wl_changer_(1);

	auto it=dfs_.find(id);

	if (dels_.find(id)!=dels_.end()) {
		throw std::runtime_error(std::string("request df after del: ")
			+ id.to_string());
	}

	if (it!=dfs_.end()) {
		NOTE("DfRequester::request " + id.to_string() + " - immediate");
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
		NOTE("DfRequester::request " + id.to_string() + " - pending");
		requests_[id].push(cb);
	}
}

void DfRequester::put(const Id &id, const ValuePtr &val, RequestCb rcb)
{
	NOTE("DfRequester::putting " + id.to_string() + "");
	std::lock_guard<std::mutex> lk(m_);
	NOTE("DfRequester::put " + id.to_string() + "");
	wl_changer_(1);
	
	auto it=dfs_.find(id);
	
	if (it!=dfs_.end() && it->second.first) {
		throw std::runtime_error(
			std::string("DF exists: " + id.to_string()));
	}

	auto it3=dels_.find(id);

	if (it3!=dels_.end()) {
		NOTE("DfRequester::put " + id.to_string() + " - del present");
		assert(requests_.find(id)==requests_.end());

		dels_.erase(it3);
		wl_changer_(-2);
		return;
	}

	dfs_[id]=std::make_pair(val, rcb);


	auto it2=requests_.find(id);

	if (it2!=requests_.end()) {
		NOTE("DfRequester::put " + id.to_string() + " - have " +
			std::to_string(it2->second.size()) + " requests");
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

std::string DfRequester::to_string() const
{
	std::lock_guard<std::mutex> lk(m_);

	std::ostringstream os;

	os << "DFs(" << dfs_.size() << "): ";
	bool need_comma=false;
	for (auto it : dfs_) {
		if (need_comma) {
			os << ", ";
		} else {
			need_comma=true;
		}
		os << it.first;
	}

	os << " Req(" << requests_.size() << "): ";
	need_comma=false;
	for (auto it : requests_) {
		if (need_comma) {
			os << ", ";
		} else {
			need_comma=true;
		}
		os << it.first << ":" << it.second.size();
	}
	os << " Dels(" << dels_.size() << "): ";
	need_comma=false;
	for (auto it : dels_) {
		if (need_comma) {
			os << ", ";
		} else {
			need_comma=true;
		}
		os << it;
	}
	return os.str();
}
