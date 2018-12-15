#pragma once

#include "code_lib.h"

class SoLib : public CodeLib
{
public:
	virtual ~SoLib();

	SoLib(const std::string &so_path);

	virtual void execute(const std::string &code,
		std::vector<Argument> &args);

private:
	std::string path_;
	void *so_;
};
