#include "value.h"

#include <cassert>
#include <cstring>

#include "stags.h"

Value::operator int() const
{
	fprintf(stderr, "Cast impossible (int): %s\n", to_string().c_str());
	abort();
}

Value::operator double() const
{
	fprintf(stderr, "Cast impossible (double): %s\n", to_string().c_str());
	abort();
}

Value::operator std::string() const
{
	fprintf(stderr, "Cast impossible (string): %s\n", to_string().c_str());
	abort();
}

Value::operator Id() const
{
	fprintf(stderr, "Cast impossible (Id): %s\n", to_string().c_str());
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
	return "int("+std::to_string(value_)+")";
}

RealValue::RealValue(double val)
	: value_(val)
{}

RealValue::RealValue(BufferPtr &buf)
{
	value_=Buffer::pop<double>(buf);
}

ValuePtr RealValue::create(double value)
{
	return ValuePtr(new RealValue(value));
}

double RealValue::value() const { return value_; }

void RealValue::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_Value_RealValue));
	bufs.push_back(Buffer::create(value_));
}

std::string RealValue::to_string() const
{
	return "real("+std::to_string(value_)+")";
}

StringValue::StringValue(const std::string &val)
	: value_(val)
{}

StringValue::StringValue(BufferPtr &buf)
{
	value_=Buffer::popString(buf);
}

ValuePtr StringValue::create(const std::string &value)
{
	return ValuePtr(new StringValue(value));
}

const std::string &StringValue::value() const { return value_; }

void StringValue::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_Value_StringValue));
	bufs.push_back(Buffer::create(value_));
}

std::string StringValue::to_string() const
{
	return "string(\""+value_+"\")";
}

JsonValue::JsonValue(const json &value)
	: value_(value)
{}

const json &JsonValue::value() const
{
	return value_;
}

void JsonValue::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(value_.dump()));
}
