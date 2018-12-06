#ifndef ID_H_
#define ID_H_

#include <memory>
#include <string>
#include <vector>

#include "printable.h"
#include "serializable.h"

typedef std::string Name;

// wrapper over vector<int>. Has optional insignificant label (doesn't
// affect comparison/equality).
// First two indices are displayed as node_rank and node-unique_id
// e.g.: (3.5423)label[1,5,7], where 1, 5, 7 are indices, and 3 and
// 5423 are "prefix" indices.
class Id : public std::vector<int>, public Serializable, public Printable
{
public:
	virtual ~Id() {}
	Id(const std::vector<int> &idx=std::vector<int>(),
		const std::string &label=Name());
	Id(BufferPtr &);

	void set_label(const Name &label);

	virtual std::string to_string() const;

	bool operator<(const Id &) const;
	bool operator==(const Id &) const;

	void serialize(Buffers &) const;

	// Modifies the reference to the tail
	static Id deserialize(Buffer &);

private:
	Name label_;
};

typedef std::shared_ptr<Id> IdPtr;

#endif // ID_H_
