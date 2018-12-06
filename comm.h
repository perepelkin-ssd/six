#ifndef COMM_H_
#define COMM_H_

#include <functional>

#include "buffer.h"

// Node identifier abstraction. Must be castable to int and constructable
// from int in [0; size) range
typedef long long int NodeId;

typedef std::function<void (const NodeId &src, const BufferPtr &data)>
	MsgHandler;

typedef std::function<void ()> Callback;

// Communication abstraction. Suits both sockets and MPI.
class Comm
{
public:
	virtual ~Comm() {};
	virtual void start()=0;
	virtual void stop()=0;
	virtual NodeId get_rank() const=0;
	virtual size_t get_size() const=0;
	virtual void send(const NodeId &dest, const Buffers &data,
		Callback cb=nullptr)=0;
	virtual MsgHandler set_handler(MsgHandler)=0;

	// broadcast to every rank, including self
	virtual void send_all(const Buffers &);

	virtual void barrier()=0;
};

#endif // COMM_H_
