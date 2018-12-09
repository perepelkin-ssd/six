#ifndef VALUE_H_
#define VALUE_H_

#include <memory>

#include "json.hpp"

#include "printable.h"
#include "serializable.h"

// DF value abstraction
class Value : public Serializable, public Printable
{
public:
	virtual ~Value() {}
};

typedef std::shared_ptr<Value> ValuePtr;

class Reference: public Value
{
};

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
private:
	int value_;
};

class JsonValue : public Value
{
public:
	virtual ~JsonValue() {}

	JsonValue(const nlohmann::json &);

	virtual const nlohmann::json &value() const;

	virtual void serialize(Buffers &) const;
private:
	nlohmann::json value_;
};

#endif // VALUE_H_
