#include "rts.h"

#include <sstream>
#include <typeindex>

#include "common.h"
#include "environ.h"
#include "logger.h"

namespace {
	std::string _prefix;
	std::string _suffix;
	std::map<int, std::string> _active;
	int _next_id=0;
	std::mutex m;
	bool _echo=false;
	void _do_echo()
	{
		std::ostringstream os;
		os << "ACTIVE: {";
		for (auto what : _active) {
			os << " " << what.second;
		}
		os << "}";
		printf("%s: %s %s\n", _prefix.c_str(), os.str().c_str(),
			_suffix.c_str());
	}
	int _add_job(const std::string &what)
	{
		std::lock_guard<std::mutex> lk(m);
		_active[_next_id]=what;
		if (_echo) {
			_do_echo();
		}
		return _next_id++;
	}
	void _rm_job(int id)
	{
		std::lock_guard<std::mutex> lk(m);
		_active.erase(id);
		if (_echo) { _do_echo(); }
	}
}

extern std::shared_ptr<Logger> L;

struct TaskEnv : public Environ, public BufHandler
{
	RTS &rts_;
	Comm &comm_;
	TaskPtr task_;
	EnvironPtr self_;
	std::weak_ptr<Environ> weak_self_;
	mutable std::mutex m_;
	std::function<void(BufferPtr &)> monitor_handler_;

	virtual void handle(BufferPtr &buf)
	{
		std::function<void(BufferPtr &)> h;
		{
			std::lock_guard<std::mutex> lk(m_);
			h=monitor_handler_;
		}

		if (h) {
			h(buf);
		} else {
			throw std::runtime_error("nullptr monitor handling");
		}
	}

	virtual ~TaskEnv() {}

	TaskEnv(RTS &rts, Comm &comm, const TaskPtr &task)
		: rts_(rts), comm_(comm), task_(task), self_(nullptr),
			monitor_handler_(nullptr)
	{}

	void init(const EnvironPtr &self)
	{
		std::lock_guard<std::mutex> lk(m_);

		weak_self_=self;
	}

	virtual Comm &comm() { return comm_; }

	virtual void send(const NodeId &dest, const Buffers &data)
	{
		int task_id=_add_job("send to " + std::to_string(dest));
		rts_.change_workload(1);
		RTS *prts=&rts_;
		Buffers bufs=data;
		bufs.push_front(Buffer::create<TAGS>(TAG_TASK));
		comm_.send(dest, bufs, [prts, task_id]() {
			prts->change_workload(-1);
			_rm_job(task_id);
		});
	}

	virtual void send_all(const Buffers &data)
	{
		for (auto i=0u; i<comm_.get_size(); i++) {
			send(NodeId(i), data);
		}
	}

	virtual void submit(const TaskPtr &task)
	{
		rts_.submit(task);
	}

	virtual Id create_id(const Name &label, const std::vector<int> &idx)
	{
		return rts_.create_id(label, idx);
	}

	virtual DfPusher &df_pusher() { return rts_.df_pusher(); }
	virtual DfRequester &df_requester() { return rts_.df_requester(); }

	virtual RPtr start_monitor(std::function<void(BufferPtr &)> h)
	{
		std::lock_guard<std::mutex> lk(m_);

		if (self_) {
			throw std::runtime_error("monitor already started");
		}

		self_=weak_self_.lock();

		if(!self_) {
			throw std::runtime_error("dead weak pointer");
		}

		assert(!monitor_handler_);
		monitor_handler_=h;

		return RPtr(comm_.get_rank(), dynamic_cast<BufHandler*>(this));
	}

	virtual void stop_monitor()
	{
		std::lock_guard<std::mutex> lk(m_);

		if (!self_) {
			throw std::runtime_error("monitor not started");
		}

		self_.reset();
		monitor_handler_=nullptr;
	}

	virtual void exec_extern(const Name &code,
		std::vector<ValuePtr> &args)
	{
		printf("EXTERN %s:", code.c_str());
		for (auto arg : args) {
			if (arg) {
				printf(" %s", arg->to_string().c_str());
			} else {
				printf(" (out)");
			}
		}
		printf("\n");
		if (code=="c_helloworld") {
			printf("Hello, six!\n");
		} else if (code=="c_init") {
			printf("c_init\n");
			args[1]=ValuePtr(new IntValue((int)(*args[0])));
		} else if (code=="c_iprint") {
			printf("c_iprint: %d\n", (int)(*args[0]));
		} else if (code=="c_rprint") {
			printf("c_rprint: %lf\n", (double)(*args[0]));
		} else if (code=="c_print") {
			printf("c_print: %s\n", args[0]->to_string().c_str());
		} else if (code=="c_show") {
			printf("c_show: %s %d\n", ((std::string)(*args[0])).c_str(),
				(int)(*args[1]));
		} else if (code=="c_hello") {
			std::string name=*args[0];
			if (name.size()>0) {
				printf("c_Hello, %s\n", name.c_str());
			} else {
				printf("c_Hello!\n");
			}
		} else {
			fprintf(stderr, "extern code not supported: %s\n",
				code.c_str());
			NIMPL
		}
	}

	virtual ValuePtr get(const Id &key) const
	{ return rts_.get(key); }
	virtual ValuePtr set(const Id &key, const ValuePtr &val)
	{ return rts_.set(key, val); }
	virtual ValuePtr del(const Id &key)
	{ return rts_.del(key); }
};

RTS::~RTS()
{
	pool_.stop();
	comm_.stop();
	delete stopper_;
}

RTS::RTS(Comm &comm)
	: comm_(comm), idle_flag_(false), workload_(0), wl_pusher_(0),
		wl_requester_(0), next_id_(0),
		df_pusher_(&pool_,
			[this](int delta) {change_workload_pusher(delta); }),
		df_requester_(&pool_,
			[this](int delta) {change_workload_requester(delta); })
{
	_prefix=std::to_string(comm_.get_rank());
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
	L->log(task->to_string());
	//printf("%d> %s\n",
	//	(int)comm_.get_rank(), task->to_string().c_str());

	change_workload(1);
	int task_id=_add_job(task->to_string());

	std::lock_guard<std::mutex> lk(m_);
		//(int)comm_.get_rank(), std::type_index(typeid(*task)).name());

	EnvironPtr env(new TaskEnv(*this, comm_, task));
	dynamic_cast<TaskEnv*>(env.get())->init(env);
	pool_.submit([env, task, this, task_id](){
		task->run(env);

		change_workload(-1);
		_rm_job(task_id);
	});
}

void RTS::change_workload(int delta)
{
	if (!delta) {
		_suffix=std::to_string(workload_)+" "+std::to_string(wl_pusher_)
			+ " " + std::to_string(wl_requester_);
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
	_suffix=std::to_string(workload_)+" "+std::to_string(wl_pusher_)
		+ " " + std::to_string(wl_requester_);
}

Id RTS::create_id(const Name &label, const std::vector<int> &idx)
{
	std::vector<int> indices;
	{
		std::lock_guard<std::mutex> lk(m_);
		indices={(int)comm_.get_rank(), next_id_++};
	}

	for (auto i : idx) {
		indices.push_back(i);
	}
	
	return Id(indices, label);
}

ValuePtr RTS::get(const Id &key) const
{
	std::lock_guard<std::mutex> lk(m_);
	
	auto it=vals_.find(key);

	if (it==vals_.end()) {
		return ValuePtr(nullptr);
	} else {
		return it->second;
	}
}

ValuePtr RTS::set(const Id &key, const ValuePtr &value)
{
	std::lock_guard<std::mutex> lk(m_);
	
	ValuePtr old_value(nullptr);

	auto it=vals_.find(key);

	if (it!=vals_.end()) {
		old_value=it->second;
	}

	vals_[key]=value;

	if (!value) {
		vals_.erase(key);
	}

	return old_value;
}


ValuePtr RTS::del(const Id &key)
{
	std::lock_guard<std::mutex> lk(m_);
	
	ValuePtr old_value(nullptr);

	auto it=vals_.find(key);

	if (it!=vals_.end()) {
		old_value=it->second;
		vals_.erase(it);
		return old_value;
	} else {
		return nullptr;
	}
}
void RTS::on_message(const NodeId &src, BufferPtr buf)
{
	TAGS tag=Buffer::pop<TAGS>(buf);
	if (tag!=TAG_IDLE_STOPPER && tag!=TAG_IDLE) {
	}
	switch (tag) {
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

void RTS::change_workload_pusher(int delta)
{
	{
		std::lock_guard<std::mutex> lk(m_);
		assert ((int)wl_pusher_>=-delta);
		wl_pusher_+=delta;
	}
	change_workload(delta);
}

void RTS::change_workload_requester(int delta)
{
	{
		std::lock_guard<std::mutex> lk(m_);
		assert ((int)wl_requester_>=-delta);
		wl_requester_+=delta;
	}
	change_workload(delta);
}
