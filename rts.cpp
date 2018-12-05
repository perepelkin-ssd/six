#include "rts.h"

#include <typeindex>

#include "common.h"
#include "environ.h"

struct TaskEnv : public Environ
{
	RTS &rts_;
	Comm &comm_;

	virtual ~TaskEnv() {}

	virtual Comm &comm() { return comm_; }

	virtual void send(const NodeId &dest, const Buffers &data)
	{
		rts_.change_workload(1);
		RTS *prts=&rts_;
		Buffers bufs=data;
		bufs.push_front(Buffer::create<TAGS>(TAG_TASK));
		comm_.send(dest, bufs, [prts]() {
			prts->change_workload(-1);
		});
		printf("%d: sending %d bytes to %d\n",
			(int)comm_.get_rank(), (int)size(data), (int)dest);
	}

	virtual void submit(const TaskPtr &task)
	{
		rts_.submit(task);
	}

	TaskEnv(RTS &rts, Comm &comm)
		: rts_(rts), comm_(comm)
	{}
	
};

RTS::~RTS()
{
	pool_.stop();
	comm_.stop();
	delete stopper_;
}

RTS::RTS(Comm &comm)
	: comm_(comm), idle_flag_(false), workload_(0)
{
	stopper_=new IdleStopper<NodeId>(
		comm_.get_rank(),
		[this](const NodeId &id) {
			NodeId next_rank((comm_.get_rank()+1)%comm_.get_size());
			comm_.send(next_rank, {
				Buffer::create(TAG_IDLE_STOPPER),
				Buffer::create(id)
			});
		},
		[this](){
			comm_.send_all({
				Buffer::create(TAG_IDLE)
			});
		}
	);

	comm_.set_handler([this](const NodeId &src, const BufferPtr &buf) {
		on_message(src, buf);
	});

	comm_.start();
	pool_.start(1);
}

//CfEnv &RTS::get_env() { return env_; }

void RTS::wait_all(bool is_root_node)
{
	if (is_root_node) {
		stopper_->start();
	}
	std::unique_lock<std::mutex> lk(m_);

	while (!idle_flag_) {
		cv_.wait(lk);
	}
}

void RTS::submit(const TaskPtr &task)
{
	printf("%d: RTS::submit: job submitted: %s\n",
		(int)comm_.get_rank(), std::type_index(typeid(*task)).name());

	change_workload(1);

	std::lock_guard<std::mutex> lk(m_);

	EnvironPtr env(new TaskEnv(*this, comm_));
	pool_.submit([env, task, this](){
//		printf("%d: RTS: running job: %s\n",
//			(int)comm_.get_rank(), std::type_index(typeid(*task)).name());
		task->run(*env);

		change_workload(-1);
	});
}

void RTS::change_workload(int delta)
{
	if (!delta) {
		return;
	}
	std::lock_guard<std::mutex> lk(m_);

	long long int new_workload=(long long int)workload_+delta;

	assert(new_workload>=0);

	if (workload_==0 && new_workload>0) {
		stopper_->set_idle(false);
	} else if (workload_!=0 && new_workload==0) {
		stopper_->set_idle(true);
	}
	workload_=(size_t)new_workload;

	printf("%d: workload: %d\n", (int)comm_.get_rank(), (int)workload_);
}

void RTS::on_message(const NodeId &src, BufferPtr buf)
{
	TAGS tag=Buffer::pop<TAGS>(buf);
	if (tag!=TAG_IDLE_STOPPER && tag!=TAG_IDLE) {
		printf("%d: on_message tag=%d from %d of size %d\n",
			(int)comm_.get_rank(), (int)tag, (int)src, (int)buf->getSize());
	}
	switch (tag) {
/*		case TAG_CF:
			env_.receive_cf(src, buf);
			break;
		case TAG_DF:
			env_.receive_df(src, buf);
			break;
		case TAG_DF_VALUE:
			env_.receive_df_to_cf(src, buf);
			break;*/
		case TAG_TASK:
			{
			auto task=TaskPtr(dynamic_cast<Task*>(factory_.construct(buf)));
			submit(task);
			}
			break;
		case TAG_IDLE_STOPPER:
			stopper_->receive(Buffer::pop<NodeId>(buf));
			break;
		case TAG_IDLE:
			{
				std::lock_guard<std::mutex> lk(m_);
				assert(!idle_flag_);
				idle_flag_=true;
				cv_.notify_all();
				break;
			}
		default:
			fprintf(stderr, "%d: RTS::on_message: invalid tag: %d\n",
				(int)comm_.get_rank(), tag);
			abort();
	}
}

