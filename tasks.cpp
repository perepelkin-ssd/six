#include "tasks.h"

#include "common.h"
#include "environ.h"

DelDf::DelDf(const Id &id)
	: id_(id)
{}

DelDf::DelDf(BufferPtr &buf)
{
	id_=Id(buf);
}

void DelDf::run(const EnvironPtr &env)
{
	env->df_requester().del(id_);
}

void DelDf::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_DelDf));
	id_.serialize(bufs);
}

std::string DelDf::to_string() const
{
	return "DelDf(" + id_.to_string() + ")";
}

Delivery::Delivery(const LocatorPtr &loc, const TaskPtr &task)
	: loc_(loc), task_(task)
{}

Delivery::Delivery(BufferPtr &buf, Factory &fact)
{
	loc_=LocatorPtr(dynamic_cast<Locator*>(fact.construct(buf)));
	assert(loc_);
	task_=TaskPtr(dynamic_cast<Task*>(fact.construct(buf)));
	assert(task_);
}

void Delivery::run(const EnvironPtr &env)
{
	auto next_node=loc_->get_next_node(env->comm());

	if (next_node==env->comm().get_rank()) {
		env->submit(task_);
	} else {
		Buffers bufs;

		serialize(bufs);
		env->send(next_node, bufs);
	}
}

void Delivery::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_Delivery));
	loc_->serialize(bufs);
	task_->serialize(bufs);
}

std::string Delivery::to_string() const
{
	return "Delivery(" + loc_->to_string() + ") : " + task_->to_string();
}

MonitorSignal::MonitorSignal(const RPtr &rptr, const Buffers &signal)
	: rptr_(rptr), signal_(signal)
{}

MonitorSignal::MonitorSignal(BufferPtr &buf)
{
	rptr_=RPtr(buf);
	signal_={buf};
}

void MonitorSignal::run(const EnvironPtr &env)
{
	if (env->comm().get_rank()==rptr_.node_) {
		BufHandler *handler=(BufHandler*)rptr_.ptr_;
		BufferPtr buf(new Buffer(signal_));
		NOTE("MONITOR: SIGNALING " + rptr_.to_string());
		handler->handle(buf);
		NOTE("MONITOR: SIGNALED " + rptr_.to_string());
	} else {
		Buffers bufs;
		serialize(bufs);
		env->send(rptr_.node_, bufs);
	}
}

void MonitorSignal::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_MonitorSignal));
	rptr_.serialize(bufs);
	for (auto s : signal_) {
		bufs.push_back(s);
	}
}

std::string MonitorSignal::to_string() const
{
	return "MonitorSignal(" + rptr_.to_string() + ")";
}

RequestDf::RequestDf(const Id &dfid, const LocatorPtr &rloc,
		const RPtr &rptr)
	: dfid_(dfid), rloc_(rloc), rptr_(rptr)
{}

RequestDf::RequestDf(BufferPtr &buf, Factory &fact)
{
	dfid_=Id(buf);
	rloc_=fact.pop<Locator>(buf);
	rptr_=RPtr(buf);
}

void RequestDf::run(const EnvironPtr &env)
{
	env->df_requester().request(dfid_, [this, env](const ValuePtr &val){
		NOTE("SENDBACK RESPONSE: " + dfid_.to_string() + " @ "
			+ rptr_.to_string());
		Buffers bufs;
		val->serialize(bufs);
		env->submit(TaskPtr(new MonitorSignal(rptr_, bufs)));
	});
}

void RequestDf::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_RequestDf));
	dfid_.serialize(bufs);
	rloc_->serialize(bufs);
	rptr_.serialize(bufs);
}

std::string RequestDf::to_string() const
{
	return "RequestDf(" + dfid_.to_string() + " => " + rloc_->to_string()
		+ ")";
}

StoreDf::StoreDf(const Id &id, const ValuePtr &val,
		const TaskPtr &on_stored, int counter)
	: id_(id), val_(val), on_stored_(on_stored), counter_(counter)
{}

StoreDf::StoreDf(BufferPtr &buf, Factory &fact)
{
	id_=Id(buf);
	val_=fact.pop<Value>(buf);
	if (Buffer::pop<bool>(buf)) {
		on_stored_=fact.pop<Task>(buf);
	} else {
		on_stored_=nullptr;
	}
	counter_=Buffer::pop<int>(buf);
}

void StoreDf::run(const EnvironPtr &env)
{
	if (counter_==-1) {
		env->df_requester().put(id_, val_);
	} else {
		std::shared_ptr<size_t> counter(new size_t(counter_));
		env->df_requester().put(id_, val_, [this, env, counter](){
			assert(*counter>0);
			(*counter)--;

			if (*counter==0) {
				env->df_requester().del(id_);
			}
		});
	}

	if (on_stored_) {
		env->submit(on_stored_);
	}
}

void StoreDf::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_StoreDf));
	id_.serialize(bufs);
	val_->serialize(bufs);
	bufs.push_back(Buffer::create<bool>((bool)on_stored_));
	if (on_stored_) {
		on_stored_->serialize(bufs);
	}
	bufs.push_back(Buffer::create<int>(counter_));
}

std::string StoreDf::to_string() const
{
	return "StoreDf(" + id_.to_string() + ") <- " + val_->to_string();
}

SubmitDfToCf::SubmitDfToCf(const Id &dfid, const ValuePtr &val,
		const Id &cfid)
	: dfid_(dfid), cfid_(cfid), val_(val)
{}

SubmitDfToCf::SubmitDfToCf(BufferPtr &buf, Factory &fact)
{
	dfid_=Id(buf);
	cfid_=Id(buf);
	val_=fact.pop<Value>(buf);
}

void SubmitDfToCf::run(const EnvironPtr &env)
{
	env->df_pusher().push(cfid_, dfid_, val_);
}

void SubmitDfToCf::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_SubmitDfToCf));
	dfid_.serialize(bufs);
	cfid_.serialize(bufs);
	val_->serialize(bufs);
}

std::string SubmitDfToCf::to_string() const
{
	return "SubmitDfToCf(" + cfid_.to_string() + " <- "
		+ dfid_.to_string() + ")";
}

