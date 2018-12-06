#pragma once

#include <memory>

#include "environ.h"
#include "printable.h"
#include "serializable.h"

class Task : public Serializable, public Printable
{
public:
	virtual ~Task() {}

	virtual void run(const EnvironPtr &)=0;

	virtual void serialize(Buffers &) const;
};

typedef std::shared_ptr<Task> TaskPtr;
