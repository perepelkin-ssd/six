#pragma once

#include <memory>

#include "task.h"

class Comm;

class Environ
{
public:
	virtual ~Environ() {}

	virtual const Comm &comm()=0;

	virtual void send(const NodeId &dest, const Buffers &data)=0;

	virtual void submit(const TaskPtr &)=0;
};

typedef std::shared_ptr<Environ> EnvironPtr;
