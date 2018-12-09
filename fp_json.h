#pragma once

#include "factory.h"
#include "task.h"

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

