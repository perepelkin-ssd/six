#pragma once

#include <memory>

#include "environ.h"
#include "serializable.h"

class Task : public Serializable
{
public:
	virtual ~Task() {}

	virtual void run(const EnvironPtr &)=0;

	virtual void serialize(Buffers &) const;
};

typedef std::shared_ptr<Task> TaskPtr;
