#pragma once

#include "buffer.h"

class Serializable
{
public:
	virtual ~Serializable() {}

	virtual void serialize(Buffers &) const=0;
};
