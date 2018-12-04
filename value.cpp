#include "value.h"

#include <cassert>
#include <cstring>

IntValue::IntValue(int val)
	: value_(val)
{}

ValuePtr IntValue::create(int value)
{
	return ValuePtr(new IntValue(value));
}

int IntValue::value() const { return value_; }

size_t IntValue::get_size() const
{
	return sizeof(value_);
}

size_t IntValue::serialize(void *buf, size_t max_len) const
{
	assert(max_len>=sizeof(value_));

	memcpy(buf, &value_, sizeof(value_));
	
	return sizeof(value_);
}
