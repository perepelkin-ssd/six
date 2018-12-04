#ifndef ID_H_
#define ID_H_

#include <memory>
#include <string>
#include <vector>

#include "serializable.h"

typedef std::string Name;

class Id : public std::vector<int>, public Serializable
{
public:
	Id(const std::vector<int> &idx=std::vector<int>(),
		const std::string &label=std::string());
	Id(BufferPtr &);

	void set_label(const std::string &label);

	std::string to_string() const;

	bool operator<(const Id &) const;
	bool operator==(const Id &) const;

	void serialize(Buffers &) const;

	// Modifies the reference to the tail
	static Id deserialize(Buffer &);

private:
	std::string label_;
};

typedef std::shared_ptr<Id> IdPtr;

#endif // ID_H_
