#pragma once

class Serializable
{
public:
	virtual ~Serializable() {}

	virtual int get_type() const=0;

	virtual size_t get_size() const=0;

	virtual size_t serialize(void *buf, size_t max_len) const=0;
};

class Factory
{
public:
	std::pair<void *, size_t> serialize(const Serializable &);
};
