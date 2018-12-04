#pragma once

#include "id.h"
#include "locator.h"
#include "serializable.h"

struct Request
{
	Id df_id;
	LocatorPtr locator;
};

class CfEnv;

class CfLogic : public Serializable
{
public:
	virtual ~CfLogic() {}

	virtual IdPtr get_label() const { return IdPtr(nullptr); }

	// Get list of DFs, which must be gathered before execution
	virtual std::vector<Request> get_requests(const CfEnv &) const
	{ return {}; };

	virtual bool is_ready(const CfEnv &) const=0;

	// Get locator specification
	virtual const LocatorPtr &get_locator(const CfEnv &) const=0;

	// Perform execution
	virtual void execute(CfEnv &)=0;
};

typedef std::shared_ptr<CfLogic> CfLogicPtr;
