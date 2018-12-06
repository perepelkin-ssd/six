#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>

#include "serializable.h"
#include "serialization_tags.h"

typedef std::function<Serializable *(BufferPtr &)> Constructor;

// Serialization/deserialization facility using Buffers
// Corresponding instance is deserialized-created provided its tag
// (serialization_tag) is assigned with a constructor.
class Factory
{
	std::map<STAGS, Constructor> cons_;
	mutable std::mutex m_;
public:
	Constructor set_constructor(STAGS tag, Constructor);

	Serializable *construct(BufferPtr &);

	// returns nullptr if none
	Constructor get_constructor(STAGS tag) const;

	// Deserialize object wrapped in a shared_ptr
	template <class T>
	std::shared_ptr<T> pop(BufferPtr &buf)
	{
		auto p=construct(buf);
		auto res=dynamic_cast<T*>(p);
		assert(res);
		return std::shared_ptr<T>(res);
	}
};


