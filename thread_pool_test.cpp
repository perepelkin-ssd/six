#include "thread_pool.h"

void thread_pool_test()
{
	ThreadPool p;
	std::mutex m;
	std::condition_variable cv;
	int sum=0;
	
	for (int i=1; i<=50; i++) {
		p.submit([&sum, &m, &cv, i](){
			std::lock_guard<std::mutex> lk(m);
			sum+=i;
			cv.notify_one();
		});
	}

	p.start(15);
	for (int i=51; i<=100; i++) {
		p.submit([&sum, &m, &cv, i](){
			std::lock_guard<std::mutex> lk(m);
			sum+=i;
			cv.notify_one();
		});
	}

	std::unique_lock<std::mutex> lk(m);
	while (sum!=5050) {
		cv.wait(lk);
	}
	lk.unlock();

	p.stop();
}
