#include "locator.h"

#include <cassert>
#include <cstring>

#include "common.h"

size_t CyclicLocator::get_size() const
{
	return sizeof(rank_);
}

size_t CyclicLocator::serialize(void *buf, size_t size) const
{
	assert(sizeof(rank_)<=size);
	memcpy(buf, &rank_, sizeof(rank_));
	return sizeof(rank_);
}

CyclicLocator::CyclicLocator(int rank)
	: rank_(rank)
{}

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
	return rank_ % size;
}

std::pair<size_t, factory::Serializable *> CyclicLocator::deserialize(
	const void *buf, size_t size)
{
	assert(size>=sizeof(int));
	auto res=std::make_pair(sizeof(int),
		new CyclicLocator(*static_cast<const int*>(buf)));
	return res;
	
}
