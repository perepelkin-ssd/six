#pragma once

#include <memory>

#include "df_env.h"

class DfLogic : public factory::Serializable
{
public:
	virtual ~DfLogic() {}

	virtual const LocatorPtr &get_locator() const=0;

	virtual void on_consumed(DfEnv &env, const Id &by_cf) {}

	virtual void on_computed(DfEnv &env, const ValuePtr &) {}
};

typedef std::shared_ptr<DfLogic> DfLogicPtr;


