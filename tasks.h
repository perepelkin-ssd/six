#pragma once

#include "factory.h"
#include "id.h"
#include "locator.h"
#include "task.h"
#include "value.h"

// Delete df from df_requester on current node
class DelDf : public Task
{
public:
	virtual ~DelDf() {}

	DelDf(const Id &dfid);
	DelDf(BufferPtr &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;
private:
	Id id_;
};

// Deliver task to another node (using locator)
class Delivery : public Task
{
public:
	virtual ~Delivery() {}

	Delivery(const LocatorPtr &, const TaskPtr &);
	Delivery(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;
private:
	LocatorPtr loc_;
	TaskPtr task_;
};

// Submit signal to a monitor using rptr
class MonitorSignal : public Task
{
public:
	virtual ~MonitorSignal() {}

	MonitorSignal(const RPtr &, const Buffers &signal);
	MonitorSignal(BufferPtr &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;
private:
	RPtr rptr_;
	Buffers signal_;
};

class RequestDf : public Task
{
public:
	virtual ~RequestDf() {}

	RequestDf(const Id &dfid, const LocatorPtr &requester_loc,
		const RPtr &);
	RequestDf(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;
private:
	Id dfid_;
	LocatorPtr rloc_;
	RPtr rptr_;
};

// Submit DF to df_requester
class StoreDf : public Task
{
public:
	virtual ~StoreDf() {}

	// counter==-1 means no counter logic
	StoreDf(const Id &dfid, const ValuePtr &val,
		const TaskPtr &on_stored=nullptr, int counter=-1);
	StoreDf(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;
private:
	Id id_;
	ValuePtr val_;
	TaskPtr on_stored_;
	int counter_;
};

// Submit df to df_pusher
class SubmitDfToCf : public Task
{
public:
	virtual ~SubmitDfToCf() {}

	SubmitDfToCf(const Id &dfid, const ValuePtr &val, const Id &cfid);
	SubmitDfToCf(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;
private:
	Id dfid_, cfid_;
	ValuePtr val_;
};
