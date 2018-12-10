#include "value.h"

#include <cassert>
#include <cstring>

#include "stags.h"

Value::operator int() const
{
	fprintf(stderr, "Cast impossible (int): %s\n", to_string().c_str());
	abort();
}

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

std::string IntValue::to_string() const
{
	return std::to_string(value_);
}

JsonValue::JsonValue(const nlohmann::json &value)
	: value_(value)
{}

const nlohmann::json &JsonValue::value() const
{
	return value_;
}

void JsonValue::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(value_.dump()));
}
