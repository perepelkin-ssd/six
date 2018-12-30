#include "value.h"

#include <cassert>
#include <cstring>

#include "common.h"
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

size_t Value::size() const
{
	fprintf(stderr, "Cannot get size: %s\n", to_string().c_str());
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

NameValue::NameValue(const Id &val)
	: value_(val)
{}

NameValue::NameValue(BufferPtr &buf)
{
	value_=Id(buf);
}

ValuePtr NameValue::create(const Id &value)
{
	return ValuePtr(new NameValue(value));
}

const Id &NameValue::value() const { return value_; }

void NameValue::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_Value_NameValue));
	value_.serialize(bufs);
}

std::string NameValue::to_string() const
{
	return "name(\""+value_.to_string()+"\")";
}

CustomValue::~CustomValue()
{
	NOTE("~Custom start");
	if (!buf_) {
		return;
	}
	bool deletion_flag=false;
	{
		NOTE("locking");
		auto id=to_string();
		std::lock_guard<std::mutex> lk(buf_->m);
		NOTE("locked");

		NOTE("XXX" + id);
		assert(buf_->refs>0);
		buf_->refs--;

		if (buf_->refs==0) {
			deletion_flag=true;
		}
	}
	if (deletion_flag) {
	NOTE("deleting");
		delete_();
	NOTE("deleted");
	}
	NOTE("~Custom end");
}

CustomValue::CustomValue(const Value &val)
{
	if (val.type()==Integer) {
		buf_.reset(new SharedBuffer());
		std::lock_guard<std::mutex> lk(buf_->m);
		buf_->data=operator new(sizeof(int));
		buf_->size=sizeof(int);
		buf_->refs=1;
		*static_cast<int*>(buf_->data)=(int)(val);
	} else if (val.type()==Real) {
		buf_.reset(new SharedBuffer());
		std::lock_guard<std::mutex> lk(buf_->m);
		buf_->data=operator new(sizeof(double));
		buf_->size=sizeof(double);
		buf_->refs=1;
		*static_cast<double*>(buf_->data)=(double)(val);
	} else if (val.type()==String) {
		buf_.reset(new SharedBuffer());
		std::lock_guard<std::mutex> lk(buf_->m);
		std::string s=(std::string)(val);
		buf_->data=operator new(s.size()+1);
		buf_->size=s.size()+1;
		buf_->refs=1;
		memcpy(buf_->data, s.c_str(), buf_->size);
	} else if (val.type()==Custom) {
		buf_=dynamic_cast<const CustomValue&>(val).buf_;
		if (!buf_) {
			ABORT("Value not set");
		}
		std::lock_guard<std::mutex> lk(buf_->m);
		assert (buf_->refs>0);
		buf_->refs++;
	} else {
		ABORT("Type not supported: " + std::to_string(val.type()));
	}
	delete_=[this](){
		default_delete();
	};
}

CustomValue::CustomValue(BufferPtr &buf)
{
	buf_.reset(new SharedBuffer());
	std::lock_guard<std::mutex> lk(buf_->m);
	buf_->size=Buffer::pop<size_t>(buf);
	buf_->data=operator new(buf_->size);
	buf_->refs=1;
	memcpy(buf_->data, buf->getData(), buf_->size);
	buf=Buffer::createSubBuffer(buf, buf_->size);
	delete_=[this](){default_delete();};
}

void CustomValue::default_delete()
{
	if (!buf_) { return; }
	{
		std::lock_guard<std::mutex> lk(buf_->m);
		assert(buf_->refs==0);
		operator delete(buf_->data);
		buf_->data=nullptr;
		buf_->size=0;
	}
	buf_.reset();
}

void CustomValue::serialize(Buffers &bufs) const
{
	if (!buf_) { ABORT("No buffer"); }
	std::lock_guard<std::mutex> lk(buf_->m);
	bufs.push_back(Buffer::create(STAG_Value_CustomValue));
	bufs.push_back(Buffer::create(buf_->size));
	bufs.push_back(BufferPtr(new Buffer(buf_->data, buf_->size)));
}

std::pair<void *, size_t> CustomValue::grab_buffer()
{
	if (!buf_) { ABORT("No buffer"); }
	std::unique_lock<std::mutex> lk(buf_->m);
	assert(buf_->refs>0);
	buf_->refs--;
	if (buf_->refs==0) {
		auto res=std::make_pair(buf_->data, buf_->size);
		lk.unlock();
		buf_.reset();
		return res;
	} else {
		// need to make a copy
		void *data=operator new(buf_->size);
		memcpy(data, buf_->data, buf_->size);
		auto res=std::make_pair(data, buf_->size);
		buf_.reset();
		return res;
	}
}

const void *CustomValue::get_data() const
{
	if (!buf_) { ABORT("No buffer"); }
	std::unique_lock<std::mutex> lk(buf_->m);
	assert(buf_->refs>0);
	return buf_->data;
}

std::string CustomValue::to_string() const
{
	if (!buf_) {
		return "value(nil)";
	}
	std::lock_guard<std::mutex> lk(buf_->m);
	return "value(" + std::to_string(buf_->size) + ": &"
		+ std::to_string(buf_->refs) + ")";
}

CustomValue *CustomValue::create_take(void *data, size_t size,
	std::function<void()> deleter)
{
	WARN("Not actually taking, but copying o_O");
	auto *res=new CustomValue();
	res->buf_.reset(new SharedBuffer());
	std::lock_guard<std::mutex> lk(res->buf_->m);
	res->buf_->size=size;
	res->buf_->data=operator new(res->buf_->size);
	res->buf_->refs=1;
	memcpy(res->buf_->data, data, res->buf_->size);
	if (!deleter) {
		res->delete_=[res](){ res->default_delete(); };
	} else {
		res->delete_=deleter;
	}
	return res;
}

CustomValue *CustomValue::create_copy(const void *data, size_t size)
{
	auto *res=new CustomValue();
	res->buf_.reset(new SharedBuffer());
	std::lock_guard<std::mutex> lk(res->buf_->m);
	res->buf_->size=size;
	res->buf_->data=operator new(res->buf_->size);
	res->buf_->refs=1;
	memcpy(res->buf_->data, data, res->buf_->size);
	res->delete_=[res](){ res->default_delete(); };
	return res;
}

