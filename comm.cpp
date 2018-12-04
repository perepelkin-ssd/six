#include "comm.h"

void Comm::send_all(const Tag &tag, const void *data, size_t size)
{
	for (NodeId rank=0; (size_t)rank<get_size(); rank++) {
		send(rank, tag, data, size);
	}
}
