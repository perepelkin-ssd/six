#include "rptr.h"

#include "serialization_tags.h"

RPtr::RPtr(const NodeId &node, void *ptr)
	: node_(node), ptr_(ptr)
{
	printf("\t\trptr(%d, %p)\n", (int)node_, ptr_);
}

RPtr::RPtr(BufferPtr &buf)
{
	node_=Buffer::pop<NodeId>(buf);
	ptr_=Buffer::pop<void*>(buf);
	printf("\t\trptr(%d, %p)\n", (int)node_, ptr_);
}

void RPtr::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(node_));
	bufs.push_back(Buffer::create(ptr_));
}
