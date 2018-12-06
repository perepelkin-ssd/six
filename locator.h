#ifndef LOCATOR_H_
#define LOCATOR_H_

#include <memory>

#include "comm.h"
#include "printable.h"
#include "serializable.h"

// An object, capable of somehow finding a node of an object in a
// distributed environment.
class Locator : public Serializable, public Printable
{
public:
	virtual ~Locator() {}

	virtual NodeId get_next_node(const Comm &) const=0;
};

// Simple locator, where node=idx % nodes_count (idx may be negative,
// where -1 is node nodes_count-1, etc.
class CyclicLocator : public Locator
{
public:
	CyclicLocator(int);
	CyclicLocator(BufferPtr &);

	virtual NodeId get_next_node(const Comm &) const;

	virtual void serialize(Buffers &) const;

	virtual std::string to_string() const;
private:
	int rank_;
};

typedef std::shared_ptr<Locator> LocatorPtr;

#endif // LOCATOR_H_
