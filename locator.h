#ifndef LOCATOR_H_
#define LOCATOR_H_

#include <memory>

#include "comm.h"
#include "serializable.h"

class Locator : public Serializable
{
public:
	virtual ~Locator() {}

	virtual NodeId get_next_node(const Comm &) const=0;
};

class CyclicLocator : public Locator
{
public:
	CyclicLocator(int);
	CyclicLocator(BufferPtr &);

	virtual NodeId get_next_node(const Comm &) const;

	virtual void serialize(Buffers &) const;
private:
	int rank_;
};

typedef std::shared_ptr<Locator> LocatorPtr;

#endif // LOCATOR_H_
