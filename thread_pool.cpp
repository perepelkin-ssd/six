#include "thread_pool.h"

#include <cassert>

#include "common.h"

ThreadPool::ThreadPool()
	: running_jobs_(0), stop_flag_(false)
{
	stop();
}

void ThreadPool::start(size_t threads_num)
{
	std::lock_guard<std::mutex> lk(m_);

	if (stop_flag_) {
		throw std::runtime_error("start while stopping ThreadPool");
	}

	for (auto i=0u; i<threads_num; i++) {
		threads_.push_back(new std::thread([this](){
			this->routine();
		}));
	}
}

void ThreadPool::stop()
{
	std::unique_lock<std::mutex> lk(m_);

	if (stop_flag_) {
		throw std::runtime_error("stop while stopping ThreadPool");
	}

	stop_flag_=true;
	cv_.notify_all();

	while (!threads_.empty()) {

		std::thread *t=threads_.back();
		threads_.pop_back();

		lk.unlock();

		t->join();
		delete t;

		lk.lock();
	}

	stop_flag_=false;
}

void ThreadPool::submit(std::function<void()> job)
{
	std::lock_guard<std::mutex> lk(m_);

	jobs_.push(job);

	NOTE("ThreadPool::submit " + std::to_string((size_t)(&job)));

	cv_.notify_one();
}

std::string ThreadPool::to_string() const
{
	std::lock_guard<std::mutex> lk(m_);

	return std::to_string(threads_.size()) + "Th "
		+ std::to_string(jobs_.size()) + "Jb "
		+ std::to_string(running_jobs_) + " RJ "
		+ (stop_flag_? "S": "");
}

void ThreadPool::routine()
{
	std::unique_lock<std::mutex> lk(m_);

	while (!stop_flag_ || !jobs_.empty() || running_jobs_>0) {
		if (jobs_.empty()) {
			cv_.wait(lk);
			continue;
		}
		
		auto job=jobs_.front();
		jobs_.pop();
		NOTE("ThreadPool::run " + std::to_string((size_t)(&job)));
		running_jobs_++;
		
		lk.unlock();

		job();

		lk.lock();

		NOTE("ThreadPool::finish " + std::to_string((size_t)(&job)));

		assert(running_jobs_>0);
		running_jobs_--;
		if (running_jobs_==0 && stop_flag_) {
			cv_.notify_all();
		}
	}
}
