#pragma once

#include <iostream>
#include <string>

class Printable
{
public:
	virtual ~Printable() {}

	virtual std::string to_string() const;
};

std::ostream &operator <<(std::ostream &, const Printable &);


