#include "task.h"

#include <typeindex>

#include "common.h"

void Task::serialize(Buffers &) const
{
	fprintf(stderr, "ERROR: serialization not implemented for %s\n",
		std::type_index(typeid(*this)).name());
	NIMPL // Unexpected serialization?
}

