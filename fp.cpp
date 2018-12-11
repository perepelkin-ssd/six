#include "fp.h"

#include "common.h"

std::set<Id> CF::get_requested_dfs(const json &cf, const Context &ctx)
{
	std::set<Id> res;

	if (cf.find("rules")==cf.end()) { return {}; }

	for (auto rule : cf["rules"]) {
		if (rule["type"]=="request") {
			for (auto ref : rule["dfs"]) {
				res.insert(ctx.eval_ref(ref));
			}
		}
	}
	return res;
}

bool CF::has_pushes(const json &cf)
{
	if (cf.find("rules")==cf.end()) { return {}; }
	for (auto rule : cf["rules"]) {
		if (rule["type"]=="has_pushes") {
			return true;
		}
	}
	return false;
}

std::set<std::pair<Id, Id> > CF::get_afterpushes(const json &cf,
	const Context &ctx)
{
	printf("CONTEXT: %s\n", ctx.to_string().c_str());
	if (cf.find("rules")==cf.end()) { return {}; }

	std::set<std::pair<Id, Id> > res;

	for (auto rule : cf["rules"]) {
		if (rule["type"]=="afterpush") {
			res.insert(std::make_pair(
				ctx.eval_ref(rule["df"]),
				ctx.eval_ref(rule["cf"])
			));
		}
	}
	return res;
}

bool CF::is_df_requested(const json &cf, const Context &ctx,
	const Id &dfid)
{
	if (cf.find("rules")==cf.end()) { return false; }
	for (auto rule : cf["rules"]) {
		printf("RULE: %s\n", rule.dump(2).c_str());
		if (rule["type"]=="req_count"
				&& ctx.eval_ref(rule["df"])==dfid) {
			assert(rule["count"]>0);
			return rule["count"]>0;
		}
	}
	return false;
}

size_t CF::get_requests_count(const json &cf, const Context &ctx,
	const Id &dfid)
{

	if (cf.find("rules")==cf.end()) {
		fprintf(stderr, "CF::get_requests_count: not available for %s"
			" in %s\n", dfid.to_string().c_str(), cf.dump(2).c_str());
		abort();
	}
	
	for (auto rule : cf["rules"]) {
		if (rule["type"]=="req_count"
				&& ctx.eval_ref(rule["df"])==dfid) {
			assert(rule["count"]>0);
			return rule["count"];
		}
	}
	fprintf(stderr, "CF::get_requests_count: not available for %s in %s\n",
		dfid.to_string().c_str(), cf.dump(2).c_str());
	abort();
}
