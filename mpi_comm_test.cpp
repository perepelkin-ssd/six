#include "mpi_comm.h"

void mpi_comm_test()
{
	delete new MpiComm(MPI_COMM_WORLD);
	Comm *pcomm=new MpiComm(MPI_COMM_WORLD);
	pcomm->start();
	pcomm->stop();
	delete pcomm;

	MPI_Barrier(MPI_COMM_WORLD);

	MpiComm comm(MPI_COMM_WORLD);


	std::mutex m;
	std::condition_variable cv;
	bool stop_flag=false;

	comm.set_handler([&m, &cv, &comm, &stop_flag](
			const NodeId &, const BufferPtr &buf) {
		std::lock_guard<std::mutex> lk(m);

		if (comm.get_rank()!=0) {
			comm.send((comm.get_rank()+1)%comm.get_size(),
				{BufferPtr(new Buffer(0))});
		}

		stop_flag=true;
		cv.notify_one();
	});

	comm.start();

	if (comm.get_rank()==0) {
		comm.send((comm.get_rank()+1)%comm.get_size(),
			{BufferPtr(new Buffer(0))});
	}

	std::unique_lock<std::mutex> lk(m);

	while (!stop_flag) {
		cv.wait(lk);
	}

	lk.unlock();

	comm.stop();

	MPI_Barrier(MPI_COMM_WORLD);
}
