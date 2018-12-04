#ifndef VALUE_H_
#define VALUE_H_

#include <memory>

#include "factory.h"

class Value : public factory::Serializable
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
	
	static ValuePtr create(int value);

	int value() const;

	virtual size_t get_size() const;
	virtual size_t serialize(void *buf, size_t) const;
private:
	int value_;
};

#endif // VALUE_H_
