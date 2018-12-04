#ifndef VALUE_H_
#define VALUE_H_

#include <memory>

#include "serializable.h"

class Value : public Serializable
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

	void serialize(Buffers &) const;
private:
	int value_;
};

#endif // VALUE_H_
