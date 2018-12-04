#ifndef MPI_COMM_H_
#define MPI_COMM_H_

#include <mpi.h>

#include <queue>

#include "comm.h"
#include "thread_pool.h"

class Message
{
public:
	Message();
	Message(const NodeId &node, const Tag &tag, const void *data,
		const size_t &size, int mpi_tag);
	void *udata();
	const void *udata() const;
	Tag &tag();
	const Tag &tag() const;

	NodeId node;
	// data buffer comprises tag and user data. buffer size is:
	// sizeof(Tag) + size
	void *data;

	// size is user data size
	size_t size;
	int mpi_tag;
};

class MpiComm : public Comm
{
public:
	virtual ~MpiComm();
	MpiComm(MPI_Comm, size_t handler_threads=1,
		size_t send_threads=1, size_t recv_threads=1,
		size_t req_threads=1);
	MpiComm(int *argc, char ***argv, size_t handler_threads=1,
		size_t send_threads=1, size_t recv_threads=1,
		size_t req_threads=1);

	virtual void start();
	virtual void stop();

	virtual NodeId get_rank() const;
	virtual size_t get_size() const;

	virtual void send(const NodeId &dest, const Tag &,
		const void *data, size_t size);
	virtual MsgHandler set_handler(MsgHandler);
private:
	static const int TAG_USER=1;
	static const int TAG_SYSTEM=2;
	NodeId rank_;
	size_t size_;
	MsgHandler handler_;
	std::queue<Message> send_queue_;
	std::mutex m_;
//	std::condition_variable cv_send_, cv_req_;
	size_t handler_threads_, send_threads_, recv_threads_, req_threads_;
	bool stop_flag_;
	ThreadPool msg_pool_, req_pool_, send_pool_, recv_pool_;
	MPI_Comm comm_;
	bool finalize_flag_;

	void receiver_routine();

	void _send(const NodeId &dest, const Tag &,
		const void *data, size_t size, int mpi_tag);
};

#endif // MPI_COMM_H_
