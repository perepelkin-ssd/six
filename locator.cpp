#include "locator.h"

#include <cassert>
#include <cstring>

#include "common.h"
#include "serialization_tags.h"

CyclicLocator::CyclicLocator(int rank)
	: rank_(rank)
{}

CyclicLocator::CyclicLocator(BufferPtr &buf)
{
	rank_=Buffer::pop<int>(buf);
}

NodeId CyclicLocator::get_next_node(const Comm &comm) const
{
	int rank=rank_, size=comm.get_size();
	if (rank<0) {
		rank=(-rank) % size;
		assert(0<=rank && rank<size);
		if (rank!=0) {
			rank=size-rank;
		}
	}
	assert(0<=rank);
	return rank % size;
}

void CyclicLocator::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_Locator_CyclicLocator));
	bufs.push_back(Buffer::create(rank_));
}

std::string CyclicLocator::to_string() const
{
	return "@" + std::to_string(rank_) + "";
}
