#pragma once

#include <set>

#include "context.h"
#include "id.h"
#include "json.h"

struct CF
{
	static std::set<Id> get_requested_dfs(const json &, const Context &);
	static bool has_pushes(const json &);
	static std::set<std::pair<Id, Id> > get_afterpushes(const json &,
		const Context &);
	static bool is_df_requested(const json &, const Context &,
		const Id &dfid);
	static bool is_requested_unlimited(const json &, const Context &,
		const Id &dfid);
	static size_t get_requests_count(const json &, const Context &,
		const Id &dfid);
	static bool is_ready(const json &fp, const json &cf, const Context &);
	static bool is_ready_exec(const json &fp, const json &cf,
		const Context &);
	static std::set<Id> get_afterdels(const json &cf, const Context &);

};

struct CFFor
{
	static bool is_unroll_at_once(const json &);
};
