#include "rptr.h"

#include <sstream>

#include "stags.h"

RPtr::RPtr(const NodeId &node, void *ptr)
	: node_(node), ptr_(ptr)
{
}

RPtr::RPtr(BufferPtr &buf)
{
	node_=Buffer::pop<NodeId>(buf);
	ptr_=Buffer::pop<void*>(buf);
}

void RPtr::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(node_));
	bufs.push_back(Buffer::create(ptr_));
}

std::string RPtr::to_string() const
{
	std::ostringstream os;

	os << std::hex << (long long)ptr_;

	return os.str() + " @" + std::to_string(node_);
}
