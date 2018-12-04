#pragma once

#include "locator.h"
#include "value.h"

class DfEnv
{
public:
	virtual ~DfEnv() {}

	virtual ValuePtr get_value(const Id &df_id) const=0;

	virtual void delete_df(const Id &df_id, const LocatorPtr &)=0;

	virtual void send_value(const LocatorPtr &dest, const Id &cf,
		const Id &df, const ValuePtr &value)=0;
};

