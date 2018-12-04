#ifndef FP_H_
#define FP_H_

#include <memory>

#include "id.h"
#include "value.h"

class Sub
{
public:
	virtual ~Sub() {}

	virtual size_t get_inputs_count() const { return 0; }
	virtual size_t get_outputs_count() const { return 0; }
};

class FP
{
public:
	virtual ~FP() {}

	virtual const Sub *get_sub(const Name &) const=0;
};

typedef std::shared_ptr<FP> FpPtr;

#endif // FP_H_
