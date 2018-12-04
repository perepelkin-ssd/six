#include "idle_stopper.h"

#include <vector>
#include <queue>

#include "common.h"

#include <unistd.h>

class FibNode
{
	FibNode *next_;
	IdleStopper<int> *stopper_;
	std::function<void()> inc_res_;
public:
	std::vector<int> jobs_;
public:
	void init(FibNode *next, IdleStopper<int> *stopper,
		std::function<void()> inc_res)
	{
		next_=next;
		stopper_=stopper;
		inc_res_=inc_res;
	}

	void push(int N)
	{
		jobs_.push_back(N);
		stopper_->set_idle(false);
	}

	void act()
	{
		if (jobs_.empty()) {
			return;
		}
		auto job=jobs_.back();
		jobs_.pop_back();
		
		assert(job>0);
		if (job<=2) {
			inc_res_();
		} else {
			next_->push(job-1);
			next_->push(job-2);
		}

		if (jobs_.empty()) {
			stopper_->set_idle(true);
		}
	}
};

void idle_stopper_test()
{
	const size_t N=10;
	IdleStopper<int> *s[N];
	FibNode n[N];
	int res=0;
	std::queue<std::pair<int, int> > msgs;
	bool done=false;

	for (auto i=0u; i<N; i++) {
		s[i]=new IdleStopper<int>(i, [&msgs, N, i](const int &id) {
			msgs.push(std::make_pair((i+1)%N, id));
		}, [&done, i]() {
			done=true;
		});
		n[i].init(&n[(i+1)%N], s[i], [&res]() { res++; });
	}

	n[0].push(7);
	s[0]->start();
	int np=0;
	int counter=0;

	while (!done) {
		n[np].act();
		while (!msgs.empty()) {
			auto msg=msgs.front();
			msgs.pop();
			s[msg.first]->receive(msg.second);
		}
		np=(np+1)%N;
		if (counter++>10000) 
			NIMPL
	}
}
