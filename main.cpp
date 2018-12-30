#include <cassert>
#include <csignal>
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
#include "so_lib.h"
#include "stags.h"
#include "task.h"
#include "tasks.h"

extern "C" {
#include "fp.h"
}

int rank;
std::string note_prefix;
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

RTS *rts;

void signal_handler(int signal)
{
	WARN("Signaled " + std::to_string(signal));
	WARN(rts->to_string());
	ABORT("SIGNALED");
}

void test_all();
int main(int argc, char **argv)
{
	int desired=MPI_THREAD_MULTIPLE, provided;
	MPI_Init_thread(&argc, &argv, desired, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	L.reset(new Logger("log.txt", std::to_string(rank), 1));
	assert(desired==provided);
	test_all();
	auto start=MPI_Wtime();

	if (argc<3 || argc>4) {
		note("Usage: %s <path/to/bfp> <path/to/so> [main_arg]\n");
	} else {
		MpiComm comm(MPI_COMM_WORLD);

		rank=comm.get_rank();

		note_prefix="";
		if (comm.get_size()<5) {
			note_prefix="\033[3" + std::to_string(2+rank) + "m";
		}
		note_prefix+=std::to_string(rank)+": ";
		if (comm.get_size()==0) {
			note_prefix="";
		}

		SoLib solib(argv[2]);

		rts=new RTS(comm, solib);
		init_stags(rts->factory());
		comm.barrier();

		std::signal(SIGINT, signal_handler);

		if (comm.get_rank()==0) {
			std::string path(argv[1]);
			long size;
			void *fp=fp_load_sz(path.c_str(), &size);
			BufferPtr fp_buf(new Buffer(fp, size, true));
			free(fp);
			//printf("executing path: %s\n", path.c_str());

			std::string main_arg="";
			if (argc==4) {
				main_arg=argv[3];
			}

			rts->submit(TaskPtr(new ExecJsonFp(fp_buf, rts->factory(),
				main_arg)));
		}

		rts->wait_all(comm.get_rank()==0);

		comm.barrier();
		delete rts;

		note("Total time: "+std::to_string(MPI_Wtime()-start)+" sec.\n");
		note("\033[38;5;29mNORMAL SYSTEM STOP\033[0m\n");
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
	BUF(STAG_Value_NameValue, NameValue)
	BUF(STAG_Value_RealValue, RealValue)
	BUF(STAG_Value_StringValue, StringValue)
	BUF(STAG_Value_CustomValue, CustomValue)

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
//	note("ThreadPool: ...");
	for (int i=0; i<2; i++) 
		thread_pool_test();
//	note("OK\n");
//	note("IdleStopper: ...");
	idle_stopper_test();
//	note("OK\n");
//	note("MpiComm: ...");
	for (int i=0; i<2; i++) {
		mpi_comm_test();
		MPI_Barrier(MPI_COMM_WORLD);
	}
//	note("OK\n");
//	note("All tests passed.\n");
#endif // NDEBUG
}

