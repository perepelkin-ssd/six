#pragma once

#include <memory>

#include "serializable.h"

class Environ;

class Task : public Serializable
{
public:
	virtual ~Task() {}

	virtual void run(Environ &)=0;

	virtual void serialize(Buffers &) const;
};

typedef std::shared_ptr<Task> TaskPtr;
