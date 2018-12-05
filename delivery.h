#pragma once

#include "factory.h"
#include "locator.h"
#include "task.h"

class Delivery : public Task
{
public:
	virtual ~Delivery() {}

	Delivery(const LocatorPtr &, const TaskPtr &);
	Delivery(BufferPtr &, Factory &);

	virtual void run(Environ &);

	virtual void serialize(Buffers &) const;
private:
	LocatorPtr loc_;
	TaskPtr task_;
};
