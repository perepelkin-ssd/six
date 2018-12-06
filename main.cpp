#include <cassert>
#include <cstring>

#include "common.h"
#include "tasks.h"
#include "environ.h"
#include "locator.h"
#include "mpi_comm.h"
#include "rts.h"
#include "serialization_tags.h"
#include "task.h"

// TEST:
/*

sub main()
{
	df x, y;
	cf a: init(x, 5);
	cf b: init(y, x+10);
	cf c: print(x, y);
	+x.on_computed: send to b
	+x.set_consumptions(2)
	+y.on_computed: send to c
	+c requests x
	+c -> y
}

*/

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

// at -1 echo("Hello!");
class MyMain1 : public Task
{
public:
	virtual void run(const EnvironPtr &)
	{
		printf("Hello!\n");
	}

	virtual void serialize(Buffers &bufs) const
	{
		bufs.push_back(Buffer::create(STAG_MyMain1));
	}
};

void test1(RTS *rts)
{
	if (rank==0) {
		TaskPtr main(new MyMain1());

		LocatorPtr loc(new CyclicLocator(-1));

		TaskPtr d_main(new Delivery(loc, main));

		rts->submit(d_main);
	}

	rts->wait_all(rank==0); // via ball
}

// at 0: main:
	// at 0 df x;
		// on_computed: send to b;
	// at 1 cf a: x:=10;
	// at 2 cf b: show(x);
	// on finish: delete x;
class My2A : public Task
{
	Id x_, b_;
	LocatorPtr xloc_, bloc_;
public:
	My2A(const Id &x, const Id &b, const LocatorPtr xloc, 
			const LocatorPtr bloc)
		:x_(x), b_(b), xloc_(xloc), bloc_(bloc)
	{}
	My2A(BufferPtr &buf, Factory &fact)
	{
		x_=Id(buf);
		b_=Id(buf);
		xloc_=fact.pop<Locator>(buf);
		bloc_=fact.pop<Locator>(buf);
	}
	virtual void run(const EnvironPtr &env)
	{
		ValuePtr xval(new IntValue(10));

		// store at xloc
		TaskPtr store(new StoreDf(x_, xval));
		env->submit(TaskPtr(new Delivery(xloc_, store)));

		// send to b
		TaskPtr sdc(new SubmitDfToCf(x_, xval, b_));
		env->submit(TaskPtr(new Delivery(bloc_, sdc)));
	}

	virtual void serialize(Buffers &bufs) const
	{
		bufs.push_back(Buffer::create(STAG_My2A));
		x_.serialize(bufs);
		b_.serialize(bufs);
		xloc_->serialize(bufs);
		bloc_->serialize(bufs);
	}
};
class My2B : public Task
{
	Id xid_, bid_;
	LocatorPtr xloc_;
	RPtr rptr_;
public:
	My2B(const Id &xid, const Id &bid, const LocatorPtr &xloc,
			const RPtr &rptr)
		: xid_(xid), bid_(bid), xloc_(xloc), rptr_(rptr)
	{}

	My2B(BufferPtr &buf, Factory &fact)
	{
		xid_=Id(buf);
		bid_=Id(buf);
		xloc_=fact.pop<Locator>(buf);
		rptr_=RPtr(buf);
	}
	virtual void run(const EnvironPtr &env)
	{
		auto bid=bid_;
		env->df_pusher().open(bid_,
				[this, env, bid](const Id &dfid, const ValuePtr &val) {
			exec(val);
			env->df_pusher().close(bid);

			// destroy df x at xloc
//			TaskPtr del_x(new DelDf(xid_));
//			TaskPtr d_del_x(new Delivery(xloc_, del_x));
//			env->submit(d_del_x);

			// event send to monitor
			TaskPtr sig_main(new MonitorSignal(rptr_,
				Buffer::create("Hello there!")));

			env->submit(sig_main);
		});
	}

	void exec(const ValuePtr &val)
	{
		int i=dynamic_cast<IntValue*>(val.get())->value();

		printf("\nx = %d\n\n", i);
	}

	virtual void serialize(Buffers &bufs) const
	{
		bufs.push_back(Buffer::create(STAG_My2B));
		xid_.serialize(bufs);
		bid_.serialize(bufs);
		xloc_->serialize(bufs);
		rptr_.serialize(bufs);
	}
};
class MyMain2 : public Task
{
public:
	virtual void run(const EnvironPtr &env)
	{
		LocatorPtr xloc(new CyclicLocator(0));
		LocatorPtr aloc(new CyclicLocator(1));
		LocatorPtr bloc(new CyclicLocator(2));

		Id xid=env->create_id("x");
		Id bid=env->create_id("b");

		auto mref=env->start_monitor([env, xloc, xid](BufferPtr &buf){
			// destroy df x at xloc
			TaskPtr del_x(new DelDf(xid));
			TaskPtr d_del_x(new Delivery(xloc, del_x));
			env->submit(d_del_x);
			env->stop_monitor();
		});

		TaskPtr a(new My2A(xid, bid, xloc, bloc));	
		TaskPtr da(new Delivery(aloc, a));
		env->submit(da);

		TaskPtr b(new My2B(xid, bid, xloc, mref));
		TaskPtr db(new Delivery(bloc, b));
		env->submit(db);

	}
};

void test2(RTS *rts)
{
	if (rank==0) {
		TaskPtr main(new MyMain2());

		rts->submit(main);
	}

	rts->wait_all(rank==0); // via ball
}

void test_all();
int main(int argc, char **argv)
{
	int desired=MPI_THREAD_MULTIPLE, provided;
	MPI_Init_thread(&argc, &argv, desired, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	assert(desired==provided);
	test_all();
	MpiComm comm(MPI_COMM_WORLD);

	rank=comm.get_rank();

	RTS *rts=new RTS(comm);

	init_stags(rts->factory());


	//test1(rts);
	MPI_Barrier(MPI_COMM_WORLD);
	test2(rts);
	MPI_Barrier(MPI_COMM_WORLD);


	delete rts;

	note("NORMAL SYSTEM STOP\n");

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

	BUF(STAG_Locator_CyclicLocator, CyclicLocator)
	BUF(STAG_Value_IntValue, IntValue)
	FACT(STAG_Delivery, Delivery)
	BUF(STAG_DelDf, DelDf)
	PLAIN(STAG_MyMain1, MyMain1)
	FACT(STAG_My2A, My2A)
	FACT(STAG_My2B, My2B)
	FACT(STAG_StoreDf, StoreDf)
	FACT(STAG_SubmitDfToCf, SubmitDfToCf)
	BUF(STAG_MonitorSignal, MonitorSignal)

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

