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

class ExecJsonFp : public Task
{
public:
	virtual ~ExecJsonFp() {}

	ExecJsonFp(const std::string &json_content);
	ExecJsonFp(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;
private:
	std::string json_dump_;
};

// Submit signal to a monitor using rptr
class MonitorSignal : public Task
{
public:
	virtual ~MonitorSignal() {}

	MonitorSignal(const RPtr &, const BufferPtr &signal);
	MonitorSignal(BufferPtr &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;
private:
	RPtr rptr_;
	BufferPtr signal_;
};

// Submit DF to df_requester
class StoreDf : public Task
{
public:
	virtual ~StoreDf() {}

	StoreDf(const Id &dfid, const ValuePtr &val,
		const TaskPtr &on_stored=nullptr);
	StoreDf(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;
private:
	Id id_;
	ValuePtr val_;
	TaskPtr on_stored_;
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


