#pragma once

#include "buffer.h"

class BufHandler
{
public:
	virtual ~BufHandler() {}

	virtual void handle(BufferPtr &)=0;
};

