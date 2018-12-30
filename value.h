#ifndef VALUE_H_
#define VALUE_H_

#include <memory>
#include <mutex>

#include "id.h"
#include "printable.h"
#include "serializable.h"

enum ValueType
{
	Integer,
	Real,
	String,
	Reference,
	Custom,
	Other
};

class Value;
typedef std::shared_ptr<Value> ValuePtr;

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

	virtual size_t size() const;
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

	virtual operator int() const { return value_; }
	virtual operator double() const { return value_; }
	virtual operator std::string() const { return std::to_string(value_); }

	virtual ValueType type() const { return Integer; }

	virtual size_t size() const { return sizeof(value_); }
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

	virtual size_t size() const { return sizeof(value_); }
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

class NameValue : public Value
{
public:
	virtual ~NameValue() {}

	NameValue(const Id &);
	NameValue(BufferPtr &buf);
	
	static ValuePtr create(const Id &);

	const Id &value() const;

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;

	virtual operator Id() const { return value_; }

	virtual ValueType type() const { return Reference; }
private:
	Id value_;
};

class CustomValue : public Value
{
	void operator=(const CustomValue &);
	CustomValue(){}
	CustomValue(const CustomValue &);
public:
	virtual ~CustomValue();

	CustomValue(const Value &);
	CustomValue(BufferPtr &);

	virtual void serialize(Buffers &) const;
	virtual ValueType type() const { return Custom; }

	std::pair<void *, size_t> grab_buffer(); // will make copy
		// if this is not the only reference

	const void *get_data() const;

	virtual std::string to_string() const;

	static CustomValue *create_copy(const void *, size_t);
	static CustomValue *create_take(void *, size_t,
		std::function<void()> delete_func=nullptr);

private:
	struct SharedBuffer
	{
		void *data;
		size_t size;
		size_t refs;
		std::mutex m;
	};

	typedef std::shared_ptr<SharedBuffer> SharedBufferPtr;
	BufferPtr buf_holder_; // TODO: optimize BufferPtr usage

	SharedBufferPtr buf_;
	std::function<void()> delete_;
	void default_delete();
};

#endif // VALUE_H_
