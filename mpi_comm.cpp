#include "mpi_comm.h"

#include <mpi.h>

#include <cassert>
#include <cstring>

#include "common.h"

Message::Message() {}

Message::Message(const NodeId &node, const Tag &tag,
		const void *data, const size_t &size, int mpi_tag)
	: node(node), size(size), mpi_tag(mpi_tag)
{
	this->data=operator new(size+sizeof(Tag));
	memcpy(this->data, &tag, sizeof(tag));
	memcpy(udata(), data, size);
}

void *Message::udata()
{
	return static_cast<char*>(data)+sizeof(Tag);
}

const void *Message::udata() const
{
	return static_cast<const char*>(data)+sizeof(Tag);
}

Tag &Message::tag()
{
	return *static_cast<Tag*>(data);
}

const Tag &Message::tag() const
{
	return *static_cast<const Tag*>(data);
}

MpiComm::~MpiComm()
{
	if (finalize_flag_) {
		MPI_Finalize();
	}
}

MpiComm::MpiComm(MPI_Comm comm, size_t handler_threads,
		size_t send_threads, size_t recv_threads, size_t req_threads)
	: handler_threads_(handler_threads), send_threads_(send_threads),
		recv_threads_(recv_threads), req_threads_(req_threads),
		stop_flag_(false), finalize_flag_(false)
{
	comm_=comm;

	int rank;
	MPI_Comm_rank(comm_, &rank);
	rank_=NodeId(rank);

	int size;
	MPI_Comm_size(comm_, &size);
	size_=size_t(size);
}

MpiComm::MpiComm(int *argc, char ***argv, size_t handler_threads,
		size_t send_threads, size_t recv_threads, size_t req_threads)
	: handler_threads_(handler_threads), send_threads_(send_threads),
		recv_threads_(recv_threads), req_threads_(req_threads),
		stop_flag_(false), finalize_flag_(true)
{
	int provided, desired=MPI_THREAD_MULTIPLE;
	MPI_Init_thread(argc, argv, desired, &provided);

	if (desired!=provided) {
		fprintf(stderr, "MPI_Init_thread failed: desired thread"
			" safety level not provided (desired: %d, got: %d)\n",
			desired, provided);
		abort();
	}

	comm_=MPI_COMM_WORLD;

	int rank;
	MPI_Comm_rank(comm_, &rank);
	rank_=NodeId(rank);

	int size;
	MPI_Comm_size(comm_, &size);
	size_=size_t(size);
}

void MpiComm::start()
{
	std::lock_guard<std::mutex> lk(m_);

	for (auto i=0u; i<recv_threads_; i++) {
		recv_pool_.submit([this](){
			receiver_routine();
		});
	}
	recv_pool_.start(recv_threads_);
	send_pool_.start(send_threads_);
	msg_pool_.start(handler_threads_);
	req_pool_.start(req_threads_);
}

void MpiComm::stop()
{
	std::unique_lock<std::mutex> lk(m_);

	assert(!stop_flag_);

	for (auto i=0u; i<recv_threads_; i++) {
		_send(rank_, 0, nullptr, 0, TAG_SYSTEM);
	}
	
	stop_flag_=true;

	lk.unlock();

	recv_pool_.stop();
	msg_pool_.stop();
	req_pool_.stop();
	send_pool_.stop();

	lk.lock();

	stop_flag_=false;
}

NodeId MpiComm::get_rank() const
{
	return rank_;
}

size_t MpiComm::get_size() const
{
	return size_;
}

void MpiComm::send(const NodeId &dest, const Tag &tag,
	const void *data, size_t size)
{
	std::lock_guard<std::mutex> lk(m_);

	if (stop_flag_) {
		fprintf(stderr, "MpiComm::send after stop()\n");
		abort();
	}

	_send(dest, tag, data, size, TAG_USER);
}
void MpiComm::_send(const NodeId &dest, const Tag &tag,
	const void *data, size_t size, int mpi_tag)
{
	Message msg(dest, tag, operator new(size), size, mpi_tag);
	memcpy(msg.udata(), data, size);

	send_pool_.submit([msg, this](){
		MPI_Request request;
		MPI_Isend(msg.data, msg.size+sizeof(Tag), MPI_BYTE,
			msg.node, msg.mpi_tag, comm_, &request);

		req_pool_.submit([msg, request](){
			MPI_Request req=request;
			MPI_Wait(&req, MPI_STATUS_IGNORE);
			operator delete(msg.data);
		});
		
	});
}

MsgHandler MpiComm::set_handler(MsgHandler handler)
{
	auto res=handler_;
	handler_=handler;
	return res;
}

void MpiComm::receiver_routine()
{
	MPI_Status status;
	MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm_, &status);

	Message msg;

	int msg_size;
	MPI_Get_count(&status, MPI_BYTE, &msg_size);
	msg.size=msg_size-sizeof(Tag);
	msg.node=status.MPI_SOURCE;
	msg.mpi_tag=status.MPI_TAG;
	msg.data=operator new(msg_size);
	MPI_Recv(msg.data, msg_size, MPI_BYTE, status.MPI_SOURCE,
		status.MPI_TAG, comm_, &status);

	msg.tag()=*(static_cast<Tag*>(msg.data));

	std::unique_lock<std::mutex> lk(m_);

	if (msg.mpi_tag==TAG_USER) {
		msg_pool_.submit([this, msg](){
			if (handler_) {
				handler_(msg.node, msg.tag(), msg.udata(),
					msg.size);
			}
			operator delete(msg.data);
		});
		recv_pool_.submit([this](){
			receiver_routine();
		});
	} else {
		assert(msg.mpi_tag==TAG_SYSTEM);
		operator delete(msg.data);
	}
}

