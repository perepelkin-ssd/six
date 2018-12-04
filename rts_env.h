#pragma once

#include <map>
#include <mutex>

#include "cf_env.h"
#include "df_env.h"

class RTS;

class RtsEnv : public CfEnv, public DfEnv
{
public:
	virtual ~RtsEnv();

	RtsEnv(RTS*);

	// CfEnv interface
	virtual ValuePtr get_df(const Id &) const;

	virtual void submit_cf(const Id &, const CfLogicPtr &);
	
	virtual void submit_df(const Id &, const DfLogicPtr &,
		const ValuePtr &);
	
	virtual void delete_df(const Id &, const LocatorPtr &);

	virtual Id create_id(const std::string &label="");

	virtual void set_on_finished(const Id &cf_id, std::function<void()>);

	// DfEnv interface
	virtual ValuePtr get_value(const Id &df_id) const;

	virtual void send_value(const LocatorPtr &dest, const Id &cf,
		const Id &df, const ValuePtr &value);
private:
	mutable std::mutex m_;
	RTS *rts_;
	int next_id_;
	// TODO: optimize: use local cf storage instead of node-wide map
	std::map<Id, std::function<void()> > on_cf_finished_;
	std::map<Id, ValuePtr> dfs_;
	std::map<Id, CfLogicPtr> cfs_;

	void transmit_cf(const NodeId &dest, const Id &id,
		const CfLogicPtr &cf);

	void transmit_df(const NodeId &dest, const Id &id,
		const DfLogicPtr &df, const ValuePtr &val);

	void transmit_df_to_cf(const NodeId &dest, const LocatorPtr &,
		const Id &cf, const Id &df, const ValuePtr &val);

	void issue_request(const Request &);

	friend class RTS;

	void receive_cf(const NodeId &src, const BufferPtr &buf);
	void receive_df(const NodeId &src, const BufferPtr &buf);
	void receive_df_to_cf(const NodeId &src, const BufferPtr &buf);
};


