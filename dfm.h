#ifndef DFM_H_
#define DFM_H_

#include <functional>
#include <memory>

#include "id.h"
#include "value.h"

class MetaDfManager
{
public:
	virtual ~MetaDfManager() {}

	virtual void request_df(const Id &,
		std::function<void(const Value&)>)=0;

	// Returns 0 if df is unavailable
	virtual const ValuePtr &get_df(const Id &)=0;

	// Upon submitting DF value a local DF manager is acquired/created
	// according to FP recoms (distr, etc.)
	// Submission is done to meta mgr, which then redirects to a particular
	// dfmgr
	virtual void submit_df(const Id &, const Value &)=0;

	// Request DF deletion
	virtual void delete_df(const Id &)=0;
};

typedef std::shared_ptr<MetaDfManager> MetaDfManagerPtr;

#endif // DFM_H_
