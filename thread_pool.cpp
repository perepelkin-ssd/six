#include "thread_pool.h"

#include <cassert>

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

	cv_.notify_one();
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
		running_jobs_++;
		
		lk.unlock();

		job();

		lk.lock();

		assert(running_jobs_>0);
		running_jobs_--;
		if (running_jobs_==0 && stop_flag_) {
			cv_.notify_all();
		}
	}
}
