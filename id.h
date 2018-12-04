#ifndef ID_H_
#define ID_H_

#include <memory>
#include <string>
#include <vector>

#include "factory.h"

typedef std::string Name;

class Id : public std::vector<int>, public factory::Serializable
{
public:
	Id(const std::vector<int> &idx=std::vector<int>(),
		const std::string &label=std::string());

	void set_label(const std::string &label);

	std::string to_string() const;

	virtual size_t get_size() const;
	virtual size_t serialize(void *buf, size_t) const;

	static std::pair<size_t, Serializable *> deserialize(const void *,
		size_t);

	bool operator<(const Id &) const;
	bool operator==(const Id &) const;

private:
	std::string label_;
};

typedef std::shared_ptr<Id> IdPtr;

#endif // ID_H_
