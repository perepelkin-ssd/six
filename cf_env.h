#ifndef CF_H_
#define CF_H_

#include <memory>

#include "cf_logic.h"
#include "df_logic.h"
#include "id.h"
#include "locator.h"
#include "value.h"

class RTS;

class CfEnv
{
public:
	virtual ~CfEnv() {}

	virtual ValuePtr get_df(const Id &) const=0;

	virtual void submit_cf(const Id &, const CfLogicPtr &)=0;
	
	virtual void submit_df(const Id &, const DfLogicPtr &,
		const ValuePtr &)=0;
	
	virtual void delete_df(const Id &, const LocatorPtr &)=0;

	virtual Id create_id(const std::string &label="")=0;

	virtual void set_on_finished(const Id &cf_id, std::function<void()>)=0;
};

#endif // CF_H_
