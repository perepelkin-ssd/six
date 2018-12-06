#include "tasks.h"

#include "common.h"
#include "environ.h"

#include "json.hpp"

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

ExecJsonFp::ExecJsonFp(const std::string &json_content)
	: json_dump_(json_content)
{}

ExecJsonFp::ExecJsonFp(BufferPtr &buf, Factory &fact)
{
	json_dump_=Buffer::popString(buf);
}

void ExecJsonFp::run(const EnvironPtr &env)
{
	nlohmann::json j=nlohmann::json::parse(json_dump_);
	printf("EXEC_JSON_FP: %s\n", j.dump(2).c_str());
}

void ExecJsonFp::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_ExecJsonFp));
	bufs.push_back(Buffer::create(json_dump_));
}

std::string ExecJsonFp::to_string() const
{
	return "(json_fp)";
}

MonitorSignal::MonitorSignal(const RPtr &rptr, const BufferPtr &signal)
	: rptr_(rptr), signal_(signal)
{
	//printf("\t\tMONITOR SIGNAL rptr(%d, %p)\n", (int)rptr.node_, rptr.ptr_);
//	printf("\t\tMONITOR SIGNAL rptr(%d, %p)\n", (int)rptr_.node_, rptr_.ptr_);
}

MonitorSignal::MonitorSignal(BufferPtr &buf)
{
	rptr_=RPtr(buf);
	signal_=buf;
//	printf("\t\tMONITOR SIGNAL rptr(%d, %p)\n", (int)rptr_.node_, rptr_.ptr_);
}

void MonitorSignal::run(const EnvironPtr &env)
{
	if (env->comm().get_rank()==rptr_.node_) {
		BufHandler *handler=(BufHandler*)rptr_.ptr_;
		handler->handle(signal_);
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
	bufs.push_back(signal_);
}

std::string MonitorSignal::to_string() const
{
	return "MonitorSignal(" + rptr_.to_string() + ")";
}

StoreDf::StoreDf(const Id &id, const ValuePtr &val,
		const TaskPtr &on_stored)
	: id_(id), val_(val), on_stored_(on_stored)
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
}

void StoreDf::run(const EnvironPtr &env)
{
	env->df_requester().put(id_, val_);

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

