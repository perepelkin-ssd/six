#pragma once

#include "factory.h"
#include "task.h"

class ExecJsonFp : public Task
{
public:
	virtual ~ExecJsonFp() {}

	ExecJsonFp(const BufferPtr &fp_buf, Factory &,
		const std::string &main_arg);
	ExecJsonFp(BufferPtr &, Factory &);

	virtual void run(const EnvironPtr &);

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;
private:
	Factory &fact_;
	BufferPtr fp_buf_;
	std::string main_arg_;
};

