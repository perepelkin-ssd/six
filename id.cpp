#include "id.h"

#include <cassert>
#include <cstring>
#include <sstream>

#include "common.h"

Id::Id(const std::vector<int> &idx, const std::string &label)
	: label_(label)
{
	for (auto i : idx) {
		push_back(i);
	}
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

size_t Id::get_size() const
{
	// serialized format:
	// <size_t label_len> <char []label> <size_t idx_count> <int []indices>
	return sizeof(size_t)+label_.size()+sizeof(size_t)+size()*sizeof(int);
}

size_t Id::serialize(void *buf, size_t buf_size) const
{
	assert(get_size()<=buf_size);

	char *cur=static_cast<char*>(buf);

	size_t idx_count=size();
	size_t label_size=label_.size();
	size_t ofs=0;

	memcpy(cur+ofs, &label_size, sizeof(label_size));
	ofs+=sizeof(label_size);
	assert(ofs<=buf_size);

	memcpy(cur+ofs, label_.c_str(), label_size); ofs+=label_size;
	assert(ofs<=buf_size);

	memcpy(cur+ofs, &idx_count, sizeof(idx_count)); ofs+=sizeof(idx_count);
	assert(ofs<=buf_size);
	for (auto i=0u; i<size(); i++) {
		int val=at(i);
		memcpy(cur+ofs, &val, sizeof(val)); ofs+=sizeof(val);
		assert(ofs<=buf_size);
	}

	return ofs;
}

std::pair<size_t, factory::Serializable *> Id::deserialize(const void *buf,
	size_t size)
{
	const char *cbuf=static_cast<const char*>(buf);
	size_t ofs=0;

	size_t label_len; assert(size-ofs>=sizeof(label_len));
	memcpy(&label_len, cbuf+ofs, sizeof(label_len)); ofs+=sizeof(label_len);

	assert(size-ofs>=label_len);
	std::string label(cbuf+ofs, label_len);
	ofs+=label_len;

	size_t idx_count; assert(size-ofs>=sizeof(idx_count));
	memcpy(&idx_count, cbuf+ofs, sizeof(idx_count)); ofs+=sizeof(idx_count);

	std::vector<int> indices;

	for (auto i=0u; i<idx_count; i++) {
		int idx; assert(size-ofs>=sizeof(idx));
		memcpy(&idx, cbuf+ofs, sizeof(idx)); ofs+=sizeof(idx);
		indices.push_back(idx);
	}

	return std::make_pair(ofs, new Id(indices, label));
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
