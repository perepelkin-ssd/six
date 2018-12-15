#include "ucenv/ucenv.h"

#include <cstring>

#include <tuple>

DF::~DF()
{
	if (type_!=Unset) {
		operator delete(data_);
	} else {
		assert(size_==0 && data_==0);
	}
}

DF::DF(const std::string &name)
	: name_(name), data_(nullptr), size_(0), type_(Unset)
{}

DF::DF(const std::string &name, int value)
	: name_(name), data_(nullptr), size_(0), type_(Unset)
{
	setValue(value);
}

DF::DF(const std::string &name, double value)
	: name_(name), data_(nullptr), size_(0), type_(Unset)
{
	setValue(value);
}

DF::DF(const std::string &name, const std::string &value)
	: name_(name), data_(nullptr), size_(0), type_(Unset)
{
	setValue(value);
}

DF::DF(const std::string &name, void *data, size_t size)
	: name_(name), data_(data), size_(size), type_(Value)
{
	if (!data) {
		fprintf(stderr, "DF: Data is nullptr: %s\n", getCName());
		abort();
	}
}

const char *DF::getCName() const { return name_.c_str(); }

size_t DF::getSize() const {
	if (type_==Unset) {
		fprintf(stderr, "DF::getSize: DF is unset: %s\n", getCName());
		abort();
	}
	assert(data_);
	return size_;
}

void DF::setValue(int val) {
	if (type_!=Unset) {
		fprintf(stderr, "DF::setValue: Reassignment not allowed: %s\n",
			getCName());
		abort();
	}
	data_=operator new(sizeof(val));
	memcpy(data_, &val, sizeof(val));
	type_=Int;
	size_=sizeof(int);
}

void DF::setValue(double val) {
	if (type_!=Unset) {
		fprintf(stderr, "DF::setValue: Reassignment not allowed: %s\n",
			getCName());
		abort();
	}
	data_=operator new(sizeof(val));
	memcpy(data_, &val, sizeof(val));
	type_=Real;
	size_=sizeof(double);
}

void DF::setValue(const std::string &val) {
	if (type_!=Unset) {
		fprintf(stderr, "DF::setValue: Reassignment not allowed: %s\n",
			getCName());
		abort();
	}
	data_=operator new(val.size()+1);
	memcpy(data_, &val, val.size()+1);
	type_=String;
	size_=val.size()+1;
}

void DF::grabBuffer(const DF &idf)
{
	if (type_!=Unset) {
		fprintf(stderr, "DF::grabBuffer: DF is already set\n");
		abort();
	}

	if (idf.type_!=Value) {
		fprintf(stderr, "DF::grabBuffer: Can only grabBuffer from "
			"Value DFs\n");
		abort();
	}

	type_=Value;
	std::tie(data_, size_)=idf.grabBuffer();
}

int DF::getInt() const
{
	if (type_!=Int) {
		fprintf(stderr, "DF::getInt: not an integer: %s (%d)\n",
			getCName(), type_);
		abort();
	}

	if (size_!=sizeof(int)) {
		fprintf(stderr, "DF::getInt: invalid size: %s (%d) %p\n",
			getCName(), (int)size_, data_);
	}
	assert(size_==sizeof(int));
	assert(data_);
	return *static_cast<const int*>(data_);
}

double DF::getReal() const
{
	if (type_!=Real) {
		fprintf(stderr, "DF::getReal: not a real: %s (%d)\n",
			getCName(), type_);
		abort();
	}

	assert(size_==sizeof(double));
	assert(data_);
	return *static_cast<const double*>(data_);
}

std::string DF::getString() const
{
	if (type_!=String) {
		fprintf(stderr, "DF::getString: not a string: %s (%d)\n",
			getCName(), type_);
		abort();
	}

	assert(data_);
	return std::string(static_cast<const char*>(data_));
}

std::tuple<void *, size_t> DF::grabBuffer() const
{
	if (type_!=Value) {
		fprintf(stderr, "DF::grabBuffer: not a 'value': %s (%d)\n",
			getCName(), type_);
	}

	type_=Unset;
	auto res=std::make_tuple(data_, size_);
	data_=nullptr; size_=0;
	return res;
}
