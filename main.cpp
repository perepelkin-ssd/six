#include <cassert>
#include <cstring>
#include <fstream>
#include <sstream>

#include "common.h"
#include "environ.h"
#include "fp_json.h"
#include "jfp.h"
#include "locator.h"
#include "logger.h"
#include "mpi_comm.h"
#include "rts.h"
#include "stags.h"
#include "task.h"
#include "tasks.h"

int rank;
void init_stags(Factory &fact);
void note(const std::string &msg)
{
	MPI_Barrier(MPI_COMM_WORLD);
	if (rank==0) {
		printf("%s", msg.c_str());
		fflush(stdout);
	}
	MPI_Barrier(MPI_COMM_WORLD);
}

std::shared_ptr<Logger> L;

void test_all();
int main(int argc, char **argv)
{
	int desired=MPI_THREAD_MULTIPLE, provided;
	MPI_Init_thread(&argc, &argv, desired, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	L.reset(new Logger("log.txt", std::to_string(rank)));
	assert(desired==provided);
	test_all();

	if (argc!=2) {
		note("no json specified, doing nothing else\n");
	} else {
		MpiComm comm(MPI_COMM_WORLD);

		rank=comm.get_rank();

		RTS *rts=new RTS(comm);
		init_stags(rts->factory());
		comm.barrier();

		if (comm.get_rank()==0) {
			std::string path(argv[1]);
			printf("executing path: %s\n", path.c_str());

			std::ifstream f(path);
			std::stringstream ss;
			ss << f.rdbuf();//read the file
			std::string j=ss.str();

			rts->submit(TaskPtr(new ExecJsonFp(j, rts->factory())));
		}

		rts->wait_all(comm.get_rank()==0);

		comm.barrier();
		delete rts;

		note("NORMAL SYSTEM STOP\n");
	}

	MPI_Finalize();
	return 0;
}

void init_stags(Factory &fact)
{
#define PLAIN(stag, cls) \
	fact.set_constructor(stag, [&](BufferPtr &) { \
		return new cls(); });
#define BUF(stag, cls) \
	fact.set_constructor(stag, [&](BufferPtr &buf) { \
		return new cls(buf); });
#define FACT(stag, cls) \
	fact.set_constructor(stag, [&fact](BufferPtr &buf) { \
		return new cls(buf, fact); });

	BUF(STAG_DelDf, DelDf)
	FACT(STAG_Delivery, Delivery)
	FACT(STAG_ExecJsonFp, ExecJsonFp)
	FACT(STAG_JfpExec, JfpExec)
	FACT(STAG_JfpReg, JfpReg)
	BUF(STAG_Locator_CyclicLocator, CyclicLocator)
	BUF(STAG_MonitorSignal, MonitorSignal)
	FACT(STAG_RequestDf, RequestDf)
	FACT(STAG_StoreDf, StoreDf)
	FACT(STAG_SubmitDfToCf, SubmitDfToCf)
	BUF(STAG_Value_IntValue, IntValue)

	for (int t=0; t<(int)_STAG_END; t++)
	{
		if(!fact.get_constructor(STAGS(t))) {
			fprintf(stderr, "STAG constructor not set: %d\n", t);
			abort();
		}
	}
#undef FACT
#undef BUF
#undef PLAIN
	MPI_Barrier(MPI_COMM_WORLD);
}

void thread_pool_test();
void idle_stopper_test();
void mpi_comm_test();
void test_mpi_factory();

void test_all()
{
#ifndef NDEBUG
	note("ThreadPool: ...");
	for (int i=0; i<2; i++) 
		thread_pool_test();
	note("OK\n");
	note("IdleStopper: ...");
	idle_stopper_test();
	note("OK\n");
	note("MpiComm: ...");
	for (int i=0; i<2; i++) {
		mpi_comm_test();
		MPI_Barrier(MPI_COMM_WORLD);
	}
	note("OK\n");
	note("All tests passed.\n");
#endif // NDEBUG
}

