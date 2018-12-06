#include "printable.h"

#include <sstream>
#include <typeinfo>

std::ostream &operator <<(std::ostream &s, const Printable &obj)
{
	s << obj.to_string();
	return s;
}

std::string Printable::to_string() const
{
	std::ostringstream os;

	os << "" << typeid(*this).name() << "";

	return os.str();
}

