#include "delivery.h"

#include "environ.h"

Delivery::Delivery(const LocatorPtr &loc, const TaskPtr &task)
	: loc_(loc), task_(task)
{}

Delivery::Delivery(BufferPtr &buf, Factory &fact)
{
	loc_=LocatorPtr(dynamic_cast<Locator*>(fact.construct(buf)));
	assert(loc_);
	task_=TaskPtr(dynamic_cast<Task*>(fact.construct(buf)));
	assert(task_);
}

void Delivery::run(Environ &env)
{
	auto next_node=loc_->get_next_node(env.comm());

	if (next_node==env.comm().get_rank()) {
		env.submit(task_);
	} else {
		Buffers bufs;

		serialize(bufs);
		env.send(next_node, bufs);
	}
}

void Delivery::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_Delivery));
	loc_->serialize(bufs);
	task_->serialize(bufs);
}
