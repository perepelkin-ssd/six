#include "value.h"

#include <cassert>
#include <cstring>

#include "serialization_tags.h"

IntValue::IntValue(int val)
	: value_(val)
{}

IntValue::IntValue(BufferPtr &buf)
{
	value_=Buffer::pop<int>(buf);
}

ValuePtr IntValue::create(int value)
{
	return ValuePtr(new IntValue(value));
}

int IntValue::value() const { return value_; }

void IntValue::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_Value_IntValue));
	bufs.push_back(Buffer::create(value_));
}
