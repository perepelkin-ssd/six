#include <cassert>
#include <cstring>

#include "common.h"
#include "fp.h"
#include "rts.h"
#include "mpi_comm.h"

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

	virtual size_t get_size() const
	{
		return loc_->get_size()+sizeof(consumptions_count_)
			+x_.get_size()+b_.get_size()+b_loc_->get_size();
	}

	virtual size_t serialize(void *buf, size_t len) const
	{
		char *cbuf=static_cast<char*>(buf);
		size_t ofs=0;
		ofs+=loc_->serialize(cbuf+ofs, len-ofs);
		
		memcpy(cbuf+ofs, &consumptions_count_, sizeof(consumptions_count_));
		ofs+=sizeof(consumptions_count_);

		ofs+=x_.serialize(cbuf+ofs, len-ofs);
		ofs+=b_.serialize(cbuf+ofs, len-ofs);
		ofs+=b_loc_->serialize(cbuf+ofs, len-ofs);

		assert(ofs<=len);

		return ofs;
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

	virtual size_t get_size() const
	{
		return y_.get_size()+c_.get_size()
			+ c_loc_->get_size();
	}

	virtual size_t serialize(void *buf, size_t len) const
	{
		NIMPL
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
	virtual size_t get_size() const NIMPL
	virtual size_t serialize(void *buf, size_t len) const NIMPL
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
	virtual size_t get_size() const
	{
		return loc_->get_size()+b_.get_size()+x_.get_size()+y_.get_size()
			+c_.get_size()+c_loc_->get_size()+y_loc_->get_size();
	}
	virtual size_t serialize(void *buf, size_t len) const
	{
		char *cbuf=static_cast<char*>(buf);
		size_t ofs=0;

		ofs+=loc_->serialize(cbuf+ofs, len-ofs); assert(ofs<=len);
		ofs+=b_.serialize(cbuf+ofs, len-ofs); assert(ofs<=len);
		ofs+=x_.serialize(cbuf+ofs, len-ofs); assert(ofs<=len);
		ofs+=y_.serialize(cbuf+ofs, len-ofs); assert(ofs<=len);
		ofs+=c_.serialize(cbuf+ofs, len-ofs); assert(ofs<=len);
		ofs+=c_loc_->serialize(cbuf+ofs, len-ofs); assert(ofs<=len);
		ofs+=y_loc_->serialize(cbuf+ofs, len-ofs); assert(ofs<=len);

		return ofs;
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
	virtual size_t get_size() const
	{
		return loc_->get_size()+c_.get_size()+x_.get_size()+y_.get_size()
			+y_loc_->get_size();
	}
	virtual size_t serialize(void *buf, size_t len) const
	{
		char *cbuf=static_cast<char*>(buf);
		size_t ofs=0;

		ofs+=loc_->serialize(cbuf+ofs, len-ofs); assert(ofs<=len);
		ofs+=c_.serialize(cbuf+ofs, len-ofs); assert(ofs<=len);
		ofs+=x_.serialize(cbuf+ofs, len-ofs); assert(ofs<=len);
		ofs+=y_.serialize(cbuf+ofs, len-ofs); assert(ofs<=len);
		ofs+=y_loc_->serialize(cbuf+ofs, len-ofs); assert(ofs<=len);

		return ofs;
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
	virtual size_t get_size() const NIMPL
	virtual size_t serialize(void *buf, size_t len) const NIMPL
};

void factory_init()
{
	factory::set_constructor(std::type_index(typeid(Id)),
		[](const void *buf, size_t size) {
			return Id::deserialize(buf, size);
	});

	factory::set_constructor(std::type_index(typeid(CyclicLocator)),
		[](const void *buf, size_t size) {
			return CyclicLocator::deserialize(buf, size);
	});
}

void test_all();
int main(int argc, char **argv)
{
	factory_init();
	int desired=MPI_THREAD_MULTIPLE, provided;
	MPI_Init_thread(&argc, &argv, desired, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	assert(desired==provided);
	test_all();
	MpiComm comm(MPI_COMM_WORLD);
	comm.start();

	rank=comm.get_rank();

	RTS rts(comm);

	auto &env=rts.get_env();
	if (rank==0) {
		Id main_id=env.create_id("main");
		CfLogicPtr cf(new MyMain(main_id));
		env.submit_cf(main_id, cf);
	}

	rts.wait_all(rank==0); // via ball
	comm.stop();

	MPI_Finalize();
	return 0;
}

void factory_test();
void thread_pool_test();
void idle_stopper_test();
void mpi_comm_test();
void test_mpi_factory();

void note(const std::string &msg)
{
	MPI_Barrier(MPI_COMM_WORLD);
	if (rank==0) {
		printf("%s", msg.c_str());
		fflush(stdout);
	}
	MPI_Barrier(MPI_COMM_WORLD);
}

void test_all()
{
#ifndef NDEBUG
	note("Factory: ...");
	factory_test();
	note("OK\n");
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

	test_mpi_factory();

	note("OK\n");
	note("All tests passed.\n");
#endif // NDEBUG
}

void test_mpi_factory()
{
	std::mutex m;
	std::condition_variable cv;
	bool flag=false;
	MpiComm comm(MPI_COMM_WORLD);
	if (comm.get_rank()+1==(int)comm.get_size()) {
		comm.set_handler([&](const NodeId &from, const Tag &,
				const void *buf, size_t size){
			std::lock_guard<std::mutex> lk(m);

			flag=true;
			cv.notify_all();

			auto res=factory::deserialize(buf, size);
			Id *id=dynamic_cast<Id *>(res.second);

			printf("ID=%s, %s\n", id->to_string().c_str(),
				(*id==Id({1,2,3}, "hello")? "eq": "neq"));

			delete id;
		});
	}
	comm.start();
	if (comm.get_rank()==0) {
		Id id({1,2,3}, "hello");
		
		char buf[get_size(id)];
		serialize(buf, get_size(id), id);
		comm.send(comm.get_size()-1, 0, buf, get_size(id));
	}

	if (comm.get_rank()+1==(int)comm.get_size()) {
		std::unique_lock<std::mutex> lk(m);

		while(!flag) {
			cv.wait(lk);
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);
	comm.stop();
}
