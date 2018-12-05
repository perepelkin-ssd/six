#pragma once

#include <functional>
#include <map>
#include <mutex>

#include "serializable.h"
#include "serialization_tags.h"

typedef std::function<Serializable *(BufferPtr &)> Constructor;

class Factory
{
	std::map<STAGS, Constructor> cons_;
	mutable std::mutex m_;
public:
	Constructor set_constructor(STAGS tag, Constructor);

	Serializable *construct(BufferPtr &);

	// returns nullptr if none
	Constructor get_constructor(STAGS tag) const;
};


