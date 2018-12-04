#include "comm.h"

void Comm::send_all(const Buffers &data)
{
	for (NodeId rank=0; (size_t)rank<get_size(); rank++) {
		send(rank, data);
	}
}
