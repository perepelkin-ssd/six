#include "mpi_comm.h"

#include <mpi.h>

#include <cassert>
#include <cstring>

#include "common.h"

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

	handler_=[](const NodeId &, const BufferPtr &){
		throw std::runtime_error("Message received before handler "
			"is set");
	}
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

	handler_=[](const NodeId &, const BufferPtr &){
		throw std::runtime_error("Message received before handler "
			"is set");
	}
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
		_send(rank_, {BufferPtr(new Buffer(0))}, TAG_SYSTEM);
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

void MpiComm::send(const NodeId &dest, const Buffers &data,
	Callback cb)
{
	if (dest<0 || dest>=(int)size_) {
		fprintf(stderr, "MpiComm::send: dest_rank out of range: 0<=%d<%d\n",
			(int)dest, (int)size_);
		abort();
	}
	std::lock_guard<std::mutex> lk(m_);

	if (stop_flag_) {
		fprintf(stderr, "MpiComm::send after stop()\n");
		abort();
	}

	_send(dest, data, TAG_USER, cb);
}
void MpiComm::_send(const NodeId &dest, const Buffers &bufs, int mpi_tag,
	Callback cb)
{
    int count=bufs.size();
    int lengths[count];
    MPI_Aint disps[count];
    MPI_Datatype old_types[count];
    MPI_Datatype mpi_type;
    int total_size=0;

    int i=0;
    char *base_ptr=bufs.empty()? nullptr: static_cast<char*>(bufs[0]->getData());
    for (auto b: bufs) {
        lengths[i]=b->getSize();
        disps[i]=static_cast<char*>(b->getData())-base_ptr;
        old_types[i]=MPI_BYTE;
        i++;
        total_size+=b->getSize();
    }
    assert(i==count);
    MPI_Type_struct(count, lengths, disps, old_types, &mpi_type);
    MPI_Type_commit(&mpi_type);

    std::shared_ptr<MPI_Request> req(new MPI_Request);

    MPI_Isend(base_ptr, 1, mpi_type, (int)dest, mpi_tag, comm_, req.get());

    MPI_Type_free(&mpi_type);

	req_pool_.submit([bufs, req, cb](){
		MPI_Wait(req.get(), MPI_STATUS_IGNORE);
		if (cb) { cb(); }
	});
}

MsgHandler MpiComm::set_handler(MsgHandler handler)
{
	auto res=handler_;
	handler_=handler;
	return res;
}

void MpiComm::barrier()
{
	MPI_Barrier(comm_);
}

void MpiComm::receiver_routine()
{
	MPI_Status status;
	MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm_, &status);

	int msg_size;
	MPI_Get_count(&status, MPI_BYTE, &msg_size);
	BufferPtr buf(new Buffer(msg_size));
	MPI_Recv(buf->getData(), msg_size, MPI_BYTE, status.MPI_SOURCE,
		status.MPI_TAG, comm_, &status);

	NodeId src(status.MPI_SOURCE);

	std::unique_lock<std::mutex> lk(m_);

	if (status.MPI_TAG==TAG_USER) {
		msg_pool_.submit([this, src, buf](){
			if (handler_) {
				handler_(src, buf);
			}
		});
		recv_pool_.submit([this](){
			receiver_routine();
		});
	} else {
		assert(status.MPI_TAG==TAG_SYSTEM);
	}
}

