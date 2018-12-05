#include <cassert>
#include <cstring>

#include "common.h"
#include "delivery.h"
#include "fp.h"
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
/*
class DfX : public DfLogic
{
	LocatorPtr loc_;
	size_t consumptions_count_;
	Id x_;
	Id b_;
	LocatorPtr b_loc_;
public:
	DfX(const Id &x, const Id &b, const LocatorPtr &b_loc)
		: loc_(new CyclicLocator(1)), x_(x), b_(b), b_loc_(b_loc) {}
	
	virtual const LocatorPtr &get_locator() const { return loc_; }

	virtual void on_consumed(DfEnv &env, const Id &cf)
	{
		consumptions_count_++;
		if (consumptions_count_==2) {
			env.delete_df(x_, loc_); // rm internal ValuePtr
		}
	}

	virtual void on_computed(DfEnv &env, const ValuePtr &val)
	{
		env.send_value(b_loc_, b_, x_, val);
		on_consumed(env, b_);
	}

	virtual void serialize(Buffers &bufs) const
	{
		loc_->serialize(bufs);
		bufs.push_back(Buffer::create(consumptions_count_));
		x_.serialize(bufs);
		b_.serialize(bufs);
		b_loc_->serialize(bufs);
	}
};

class DfY : public DfLogic
{
	LocatorPtr y_loc_;
	Id y_;
	Id c_;
	LocatorPtr c_loc_;
public:
	DfY(const LocatorPtr &y_loc, const Id &y, const Id &c,
		const LocatorPtr &c_loc)
		: y_loc_(y_loc), y_(y), c_(c), c_loc_(c_loc) {}

	virtual const LocatorPtr &get_locator() const { return y_loc_; }

	virtual void on_computed(DfEnv &env, const ValuePtr &val)
	{
		env.send_value(c_loc_, c_, y_, val);
	}

	virtual void serialize(Buffers &bufs) const
	{
		y_loc_->serialize(bufs);
		y_.serialize(bufs);
		c_.serialize(bufs);
		c_loc_->serialize(bufs);
	}
};

class CfA : public CfLogic
{
	LocatorPtr loc_;
	Id a_, x_, b_;
	LocatorPtr b_loc_;
public:
	CfA(const Id &a, const Id &x, const Id &b, const LocatorPtr &b_loc)
		: loc_(new CyclicLocator(0)), a_(a), x_(x), b_(b), b_loc_(b_loc)
	{}

	virtual bool is_ready(const CfEnv &env) const { return true; }

	virtual const LocatorPtr &get_locator(const CfEnv &) const
	{ return loc_; }

	virtual void execute(CfEnv &env)
	{
		env.submit_df(x_, DfLogicPtr(new DfX(x_, b_, b_loc_)),
			IntValue::create(5));
	}

	void serialize(Buffers &bufs) const
	{
		loc_->serialize(bufs);
		a_.serialize(bufs);
		x_.serialize(bufs);
		b_.serialize(bufs);
		b_loc_->serialize(bufs);
	}
};

class CfB : public CfLogic
{
	LocatorPtr loc_;
	Id b_, x_, y_, c_;
	LocatorPtr c_loc_, y_loc_;
public:
	CfB(const LocatorPtr &loc, const Id &b, const Id &x, const Id &y,
		const Id &c, const LocatorPtr &c_loc, const LocatorPtr &y_loc)
		: loc_(loc), b_(b), x_(x), y_(y), c_(c),
			c_loc_(c_loc), y_loc_(y_loc)
	{}

	virtual bool is_ready(const CfEnv &env) const
	{
		return env.get_df(x_).get()!=nullptr;
	}

	virtual const LocatorPtr &get_locator(const CfEnv &) const
	{ return loc_; }

	virtual void execute(CfEnv &env)
	{
		auto x_val=dynamic_cast<IntValue*>(env.get_df(x_).get())->value();

		env.submit_df(y_, DfLogicPtr(new DfY(y_loc_, y_, c_, c_loc_)),
			IntValue::create(10+x_val));
	}
	void serialize(Buffers &bufs) const
	{
		loc_->serialize(bufs);
		b_.serialize(bufs);
		x_.serialize(bufs);
		y_.serialize(bufs);
		c_.serialize(bufs);
		c_loc_->serialize(bufs);
		y_loc_->serialize(bufs);
	}
};

class CfC : public CfLogic
{
	LocatorPtr loc_;
	Id c_, x_, y_;
	LocatorPtr y_loc_;
public:
	CfC(const LocatorPtr &loc, const Id &c, const Id &x, const Id &y,
		const LocatorPtr &y_loc)
		: loc_(loc), c_(c), x_(x), y_(y), y_loc_(y_loc)
	{}

	virtual bool is_ready(const CfEnv &env) const
	{
		return env.get_df(x_).get()!=nullptr
			&& env.get_df(y_).get()!=nullptr;
	}

	virtual const LocatorPtr &get_locator(const CfEnv &) const
	{ return loc_; }

	virtual void execute(CfEnv &env)
	{
		auto x_val=dynamic_cast<IntValue*>(env.get_df(x_).get())->value();
		auto y_val=dynamic_cast<IntValue*>(env.get_df(y_).get())->value();

		printf("x=%d, y=%d\n", x_val, y_val);

		env.delete_df(y_, y_loc_);
	}

	void serialize(Buffers &bufs) const
	{
		loc_->serialize(bufs);
		c_.serialize(bufs);
		x_.serialize(bufs);
		y_.serialize(bufs);
		y_loc_->serialize(bufs);
	}
};

class MyMain : public CfLogic
{
	LocatorPtr loc_;
	Id my_id_;
public:
	MyMain(const Id &my_id) : loc_(new CyclicLocator(0)), my_id_(my_id) {}

	virtual bool is_ready(const CfEnv &) const { return true; }

	virtual const LocatorPtr &get_locator(const CfEnv &) const
	{ return loc_; }

	virtual void execute(CfEnv &env)
	{
		Id x=env.create_id("x");
		Id y=env.create_id("y");
		Id a=env.create_id("a");
		Id b=env.create_id("b");
		Id c=env.create_id("c");

		auto b_loc=LocatorPtr(new CyclicLocator(2));
		auto c_loc=LocatorPtr(new CyclicLocator(4));
		auto y_loc=LocatorPtr(new CyclicLocator(3));

		CfLogicPtr al(new CfA(a, x, b, b_loc));
		CfLogicPtr bl(new CfB(b_loc, b, x, y, c, c_loc, y_loc));
		CfLogicPtr cl(new CfC(c_loc, c, x, y, y_loc));

		env.set_on_finished(my_id_, [](){
			printf("finished\n");
		});

		env.submit_cf(a, al);
		env.submit_cf(b, bl);
		env.submit_cf(c, cl);
	}
	virtual void serialize(Buffers &) const NIMPL
};
*/
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
	virtual void run(Environ &)
	{
		printf("Hello!\n");
	}

	virtual void serialize(Buffers &bufs) const
	{
		bufs.push_back(Buffer::create(STAG_MyMain1));
	}
};

// at 0: main:
	// at 0 df x;
		// on_computed: send to b;
	// at 1 cf a: x:=10;
	// at 2 cf b: show(x);
	// on finish: delete x;
class MyMain2 : public Task
{
public:
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

	test1(rts);
	test1(rts);


	delete rts;

	note("NORMAL SYSTEM STOP\n");

	MPI_Finalize();
	return 0;
}

void init_stags(Factory &fact)
{
	fact.set_constructor(STAG_Locator_CyclicLocator, [](BufferPtr &buf){
		return new CyclicLocator(buf);
	});

	fact.set_constructor(STAG_Value_IntValue, [](BufferPtr &buf){
		return new IntValue(buf);
	});

	fact.set_constructor(STAG_Delivery, [&fact](BufferPtr &buf){
		return new Delivery(buf, fact);
	});

	fact.set_constructor(STAG_MyMain1, [](BufferPtr &) {
		return new MyMain1();
	});

	for (int t=0; t<(int)_STAG_END; t++)
	{
		if(!fact.get_constructor(STAGS(t))) {
			fprintf(stderr, "STAG constructor not set: %d\n", t);
			abort();
		}
	}
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

