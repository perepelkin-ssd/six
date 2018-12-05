#pragma once

#include "factory.h"
#include "id.h"
#include "locator.h"
#include "task.h"
#include "value.h"

class DelDf : public Task
{
public:
	virtual ~DelDf() {}

	DelDf(const Id &dfid);
	DelDf(BufferPtr &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;
private:
	Id id_;
};

class Delivery : public Task
{
public:
	virtual ~Delivery() {}

	Delivery(const LocatorPtr &, const TaskPtr &);
	Delivery(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;
private:
	LocatorPtr loc_;
	TaskPtr task_;
};

class MonitorSignal : public Task
{
public:
	virtual ~MonitorSignal() {}

	MonitorSignal(const RPtr &, const BufferPtr &signal);
	MonitorSignal(BufferPtr &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;
private:
	RPtr rptr_;
	BufferPtr signal_;
};

class StoreDf : public Task
{
public:
	virtual ~StoreDf() {}

	StoreDf(const Id &dfid, const ValuePtr &val,
		const TaskPtr &on_stored=nullptr);
	StoreDf(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;
private:
	Id id_;
	ValuePtr val_;
	TaskPtr on_stored_;
};

class SubmitDfToCf : public Task
{
public:
	virtual ~SubmitDfToCf() {}

	SubmitDfToCf(const Id &dfid, const ValuePtr &val, const Id &cfid);
	SubmitDfToCf(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;

private:
	Id dfid_, cfid_;
	ValuePtr val_;
};


