#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class ThreadPool
{
public:
	ThreadPool();

	// start (more) threads
	// Note: must not start while stop() is in progress
	void start(size_t threads_num=1);

	// request threads stop and join them (also waits for the queue to
	// become empty)
	void stop();

	// add job
	void submit(std::function<void()>);

private:
	std::mutex m_;
	std::condition_variable cv_;
	std::vector<std::thread*> threads_;
	std::queue<std::function<void()> > jobs_;
	size_t running_jobs_;
	bool stop_flag_;

	void routine();
};

#endif // THREAD_POOL_H_
