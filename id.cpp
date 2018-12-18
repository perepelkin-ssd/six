#include "id.h"

#include <cassert>
#include <cstring>
#include <sstream>

#include "common.h"

Id::Id(const std::vector<int> &idx, const Name &label)
	: label_(label)
{
	for (auto i : idx) {
		push_back(i);
	}
}

Id::Id(BufferPtr &buf)
{
	size_t indices_count=Buffer::pop<size_t>(buf);

	for (auto i=0u; i<indices_count; i++) {
		push_back(Buffer::pop<int>(buf));
	}

	label_=Buffer::popString(buf);
}

std::string Id::to_string() const
{
	std::ostringstream os;

	if (size()==0) {
		os << "()" << label_;
		return os.str();
	} else if (size()==1) {
		os << "(" << this->at(0) << ")" << label_;
		return os.str();
	}

	os << "(" << this->at(0) << "." << this->at(1) << ")";

	os << label_;

	if (size()>2) {
		os << "[";
		for (auto i=2u; i<size(); i++) {
			if (i>2) {
				os << ",";
			}
			os << this->at(i);
		}
		os << "]";
	}
	return os.str();
}

bool Id::operator<(const Id &id) const
{
	if (size() < id.size()) return true;
	if (size() > id.size()) return false;

	for (auto i=0u; i<size(); i++) {
		if (at(i)<id.at(i)) { return true; }
		if (at(i)>id.at(i)) { return false; }
	}

	return false;
}

bool Id::operator==(const Id &id) const
{
	if (size() != id.size()) { return false; }

	for (auto i=0u; i<size(); i++) {
		if (at(i)!=id.at(i)) { return false; }
	}

	return true;
}

void Id::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create<size_t>(size()));
	
	bufs.push_back(BufferPtr(new Buffer(data(), size()*sizeof(int))));

	bufs.push_back(Buffer::create(label_));
}

bool Id::starts_with(const Id &prefix) const
{
	if (prefix.size()>size()) {
		return false;
	}

	for (auto i=0u; i<prefix.size(); i++) {
		if (prefix.at(i)!=at(i)) {
			return false;
		}
	}

	return true;
}
