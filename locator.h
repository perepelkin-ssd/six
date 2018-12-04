#ifndef LOCATOR_H_
#define LOCATOR_H_

#include <memory>

#include "comm.h"
#include "factory.h"

class Locator : public factory::Serializable
{
public:
	virtual ~Locator() {}

	virtual NodeId get_next_node(const Comm &) const=0;
};

class CyclicLocator : public Locator
{
public:
	CyclicLocator(int);

	virtual NodeId get_next_node(const Comm &) const;

	virtual size_t get_size() const;
	virtual size_t serialize(void *dest, size_t max_len) const;

	static std::pair<size_t, factory::Serializable *> deserialize(
		const void *, size_t);
private:
	int rank_;
};

typedef std::shared_ptr<Locator> LocatorPtr;

#endif // LOCATOR_H_
