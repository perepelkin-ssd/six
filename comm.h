#ifndef COMM_H_
#define COMM_H_

#include <functional>

#include "buffer.h"

typedef long long int NodeId;
typedef int Tag;

typedef std::function<
	void (const NodeId &src, const Tag &, const void *, size_t)
> MsgHandler;

// Communication abstraction. Suits both sockets and MPI.
class Comm
{
public:
	virtual ~Comm() {};
	virtual void start()=0;
	virtual void stop()=0;
	virtual NodeId get_rank() const=0;
	virtual size_t get_size() const=0;
	virtual void send(const NodeId &dest, const Tag &,
		const void *data, size_t size)=0;
	virtual MsgHandler set_handler(MsgHandler)=0;

	virtual void send_all(const Tag &, const void *data, size_t size);
};

#endif // COMM_H_
