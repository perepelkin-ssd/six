#ifndef VALUE_H_
#define VALUE_H_

#include <memory>

#include "id.h"
#include "json.h"
#include "printable.h"
#include "serializable.h"

enum ValueType
{
	Integer,
	Real,
	String,
	Reference,
	Other
};

// DF value abstraction
class Value : public Serializable, public Printable
{
public:
	virtual ~Value() {}

	virtual operator int() const;
	virtual operator double() const;
	virtual operator std::string() const;
	virtual operator Id() const;

	virtual ValueType type() const=0;
};

typedef std::shared_ptr<Value> ValuePtr;

class IntValue : public Value
{
public:
	virtual ~IntValue() {}

	IntValue(int);
	IntValue(BufferPtr &buf);
	
	static ValuePtr create(int value);

	int value() const;

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;

	virtual operator int() const { return value_; }
	virtual operator double() const { return value_; }
	virtual operator std::string() const { return std::to_string(value_); }

	virtual ValueType type() const { return Integer; }
private:
	int value_;
};

class RealValue : public Value
{
public:
	virtual ~RealValue() {}

	RealValue(double);
	RealValue(BufferPtr &buf);
	
	static ValuePtr create(double value);

	double value() const;

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;

	virtual operator int() const { return value_; }
	virtual operator double() const { return value_; }
	virtual operator std::string() const { return std::to_string(value_); }

	virtual ValueType type() const { return Real; }
private:
	double value_;
};

class StringValue : public Value
{
public:
	virtual ~StringValue() {}

	StringValue(const std::string &);
	StringValue(BufferPtr &buf);
	
	static ValuePtr create(const std::string &value);

	const std::string &value() const;

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;

	virtual operator std::string() const { return value_; }

	virtual ValueType type() const { return String; }
private:
	std::string value_;
};

class JsonValue : public Value
{
public:
	virtual ~JsonValue() {}

	JsonValue(const json &);

	virtual const json &value() const;

	virtual void serialize(Buffers &) const;

	virtual ValueType type() const { return Other; }
private:
	json value_;
};

#endif // VALUE_H_
