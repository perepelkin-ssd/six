#pragma once

#include <set>

#include "context.h"
#include "id.h"
#include "locator.h"

struct CF
{
	static std::set<Id> get_requested_dfs(const void *cf, const Context &);

	static bool has_pushes(const void *cf);
	
	static std::set<std::pair<Id, Id> > get_afterpushes(const void *cf,
		const Context &);
	
	static bool is_df_requested(const void *cf, const Context &,
		const Id &dfid);
	
	static bool is_requested_unlimited(const void *cf, const Context &,
		const Id &dfid);
	
	static size_t get_requests_count(const void *cf, const Context &,
		const Id &dfid);
	
	static bool is_ready(const void *fp, const void *cf, const Context &);
	
	static bool is_ready_exec(const void *fp, const void *cf,
		const Context &);
	
	static std::set<Id> get_afterdels(const void *cf, const Context &);
	
	static std::set<Id> get_aftersets(const void *cf, const Context &);
	
	static LocatorPtr get_locator(const void *cf, const LocatorPtr &base,
		const Context &ctx);
	
	static std::map<Id, const void *> get_global_locators(const void *rules,
		const Context &);
	// TODO: improve interface
	
	static LocatorPtr get_global_locator(const void *loc_spec,
		const std::vector<int> &indices, const Context &);
	
	static const void *get_loc_spec(const void *cf);
	// in format:
	//	"ref": ["x", {..i..}],
	//	"expr": {..i..}
	//	"type": "locator_cyclic"
};

struct CFFor
{
	static bool is_unroll_at_once(const void *);
};
