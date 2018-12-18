#pragma once

#include <string>
#include <vector>

#include "value.h"

class CodeLib
{
public:
	typedef std::tuple<
		ValueType,
		ValuePtr,
		Id
	> Argument;
	~CodeLib() {}

	virtual void execute(const std::string &code,
		std::vector<Argument> &args)=0;
		
};
