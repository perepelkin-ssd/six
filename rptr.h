#pragma once

#include "comm.h"
#include "printable.h"
#include "serializable.h"

// Remote pointer, i.e. node+pointer pair.
class RPtr : public Serializable, public Printable
{
public:
	virtual ~RPtr() {}

	RPtr(const NodeId &nid=NodeId(-1), void *ptr=nullptr);
	RPtr(BufferPtr &);

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;
public:
	NodeId node_;
	void *ptr_;
};

