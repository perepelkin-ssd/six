#pragma once

#include "comm.h"
#include "serializable.h"

class RPtr : public Serializable
{
public:
	virtual ~RPtr() {}

	RPtr(const NodeId &nid=NodeId(-1), void *ptr=nullptr);
	RPtr(BufferPtr &);

	virtual void serialize(Buffers &) const;

public:
	NodeId node_;
	void *ptr_;
};

