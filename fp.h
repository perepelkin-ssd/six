#pragma once

#include <set>

#include "context.h"
#include "id.h"
#include "json.h"

struct CF
{
public:
	static std::set<Id> get_requested_dfs(const json &, const Context &);
	static bool has_pushes(const json &);
	static std::set<std::pair<Id, Id> > get_afterpushes(const json &,
		const Context &);
	static bool is_df_requested(const json &, const Context &,
		const Id &dfid);
	static size_t get_requests_count(const json &, const Context &,
		const Id &dfid);
};
