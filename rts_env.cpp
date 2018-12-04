#include "rts_env.h"

#include <cassert>

#include "common.h"
#include "rts.h"

RtsEnv::~RtsEnv() {}

RtsEnv::RtsEnv(RTS *rts)
	: rts_(rts), next_id_(0)
{}

ValuePtr RtsEnv::get_df(const Id &) const NIMPL

void RtsEnv::submit_cf(const Id &id, const CfLogicPtr &cf)
{
	LocatorPtr loc=cf->get_locator(*this);

	NodeId next_node=loc->get_next_node(rts_->comm_);

	if (next_node!=rts_->comm_.get_rank()) {
		printf("%d: cf %s must be transferred to node %d\n",
			(int)rts_->comm_.get_rank(), id.to_string().c_str(),
			(int)next_node);
		transmit_cf(next_node, id, cf);
		return;
	}

	for (auto req : cf->get_requests(*this)) {
		issue_request(req);
	}

	if (cf->is_ready(*this)) {
		printf("%d: EXECUTE_CF: %s\n", (int)rts_->comm_.get_rank(),
			id.to_string().c_str());
		cf->execute(*this);
	}

	std::lock_guard<std::mutex> lk(m_);

	if (cfs_.find(id)!=cfs_.end()) {
		fprintf(stderr, "%d: RtsEnv::submit_cf: duplicate cf: %s\n",
			(int)rts_->comm_.get_rank(), id.to_string().c_str());
		abort();
	}

	cfs_[id]=cf;
}

void RtsEnv::submit_df(const Id &id, const DfLogicPtr &df,
	const ValuePtr &val)
{
	NodeId dest_node=df->get_locator()->get_next_node(rts_->comm_);

	df->on_computed(*this, val);

	if (dest_node != rts_->comm_.get_rank()) {
		transmit_df(dest_node, id, df, val);
		return;
	}

	NIMPL
	// Algorithm: store smart-ptr here
	// handle events according to df_logic
	// upon submission check awaiting requests on board
}

void RtsEnv::delete_df(const Id &, const LocatorPtr &) NIMPL

Id RtsEnv::create_id(const std::string &label)
{
	std::lock_guard<std::mutex> lk(m_);
	return Id({(int)rts_->comm_.get_rank(), next_id_++}, label);
}

void RtsEnv::set_on_finished(const Id &cf_id,
	std::function<void()> handler)
{
	std::lock_guard<std::mutex> lk(m_);

	if (on_cf_finished_.find(cf_id)!=on_cf_finished_.end()) {
		fprintf(stderr, "%d: RtsEnv::set_on_finished: "
			"already set for cf: %s\n", (int)rts_->comm_.get_rank(),
			cf_id.to_string().c_str());
		abort();
	}

	on_cf_finished_[cf_id]=handler;
}

ValuePtr RtsEnv::get_value(const Id &df_id) const
{
	std::lock_guard<std::mutex> lk(m_);
	
	if (dfs_.find(df_id)==dfs_.end()) {
		fprintf(stderr, "%d: RtsEnv::get_value: no such df: %s\n",
			(int)rts_->comm_.get_rank(), df_id.to_string().c_str());
		print_trace();
		abort();
	}

	return dfs_.find(df_id)->second;
}

void RtsEnv::send_value(const LocatorPtr &dest, const Id &cf,
	const Id &df, const ValuePtr &value)
{
	NodeId next_node=dest->get_next_node(rts_->comm_);

	if (next_node!=rts_->comm_.get_rank()) {
		transmit_df_to_cf(next_node, dest, cf, df, value);
		return;
	}

	// Algorithm:
	// if cf exists, recheck its ready status
	// else postpone this delivery until cf with such id appears
}

void RtsEnv::transmit_cf(const NodeId &dest, const Id &id,
	const CfLogicPtr &cf)
{
	Buffers bufs;

	id.serialize(bufs);
	cf->serialize(bufs);

	bufs.push_front(Buffer::create(RTS::TAG_CF));
	rts_->comm_.send(dest, bufs);
}

void RtsEnv::receive_cf(const NodeId &src, const BufferPtr &buf)
{
	BufferPtr b(buf);
	Id id=Buffer::popItem<Id>(b);

	NIMPL /*
	printf("START\n");
	const char *cbuf=static_cast<const char *>(buf);
	size_t ofs=0;
	std::pair<size_t, factory::Serializable *> res;

	printf("START 1\n");
	res=factory::deserialize(cbuf+ofs, size-ofs); ofs+=res.first;
	printf("START 2\n");
	IdPtr id(dynamic_cast<Id*>(res.second)); assert(id.get());
	printf("START 3\n");

	printf("START 4\n");
	res=factory::deserialize(cbuf+ofs, size-ofs); ofs+=res.first;
	printf("START 5\n");
	CfLogicPtr cf(dynamic_cast<CfLogic*>(res.second)); assert(cf.get());
	printf("START 6\n");

	assert(ofs==size);

	printf("%d: received cf `%s` from %d\n", (int)rts_->comm_.get_rank(),
		id->to_string().c_str(), (int)src);
	printf("SUBMIT\n");
	submit_cf(*id, cf);
	printf("END\n");*/
}

void RtsEnv::transmit_df(const NodeId &dest, const Id &id,
	const DfLogicPtr &df, const ValuePtr &val)
{
	Buffers bufs;

	id.serialize(bufs);
	df->serialize(bufs);
	val->serialize(bufs);

	bufs.push_front(Buffer::create(RTS::TAG_DF));
	rts_->comm_.send(dest, bufs);
}

void RtsEnv::receive_df(const NodeId &src, const BufferPtr &buf)
{
	NIMPL /*
	const char *cbuf=static_cast<const char *>(buf);
	size_t ofs=0;
	std::pair<size_t, factory::Serializable *> res;

	res=factory::deserialize(cbuf+ofs, size-ofs); ofs+=res.first;
	IdPtr id(dynamic_cast<Id*>(res.second)); assert(id.get());

	res=factory::deserialize(cbuf+ofs, size-ofs); ofs+=res.first;
	DfLogicPtr df(dynamic_cast<DfLogic*>(res.second)); assert(df.get());

	res=factory::deserialize(cbuf+ofs, size-ofs); ofs+=res.first;
	ValuePtr val(dynamic_cast<Value*>(res.second)); assert(val.get());

	assert(ofs==size);

	printf("%d: received df `%s` from %d\n", (int)rts_->comm_.get_rank(),
		id->to_string().c_str(), (int)src);
	submit_df(*id, df, val);
*/
}

void RtsEnv::transmit_df_to_cf(const NodeId &dest, const LocatorPtr &loc,
	const Id &cf, const Id &df, const ValuePtr &val)
{
	Buffers bufs;

	loc->serialize(bufs);
	cf.serialize(bufs);
	df.serialize(bufs);
	val->serialize(bufs);

	bufs.push_front(Buffer::create(RTS::TAG_DF_VALUE));
	rts_->comm_.send(dest, bufs);
}

void RtsEnv::receive_df_to_cf(const NodeId &src, const BufferPtr &cbuf)
{
	BufferPtr buf(cbuf);
	LocatorPtr loc(dynamic_cast<Locator*>(rts_->construct(buf)));
	Id cf=Buffer::popItem<Id>(buf);
	Id df=Buffer::popItem<Id>(buf);
	ValuePtr val(dynamic_cast<Value*>(rts_->construct(buf)));

	send_value(loc, cf, df, val);
}

void RtsEnv::issue_request(const Request &req)
{
	NIMPL // implement a request board, transmit if needed, and submit
	// to board
}

