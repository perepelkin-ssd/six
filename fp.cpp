#include "fp.h"

#include "common.h"

std::set<Id> CF::get_requested_dfs(const json &cf, const Context &ctx)
{
	std::set<Id> res;

	if (cf.find("rules")==cf.end()) { return {}; }

	for (auto rule : cf["rules"]) {
		if (rule["type"]=="request") {
			for (auto ref : rule["dfs"]) {
				if (ctx.can_eval_ref(ref)) {
					res.insert(ctx.eval_ref(ref));
				}
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

bool CF::is_ready(const json &fp, const json &cf, const Context &ctx)
{
	if (cf["type"]=="exec") {
		return CF::is_ready_exec(fp, cf, ctx);
	} else {
		fprintf(stderr, "JfpExec::get_args: unsupported cf type: %s\n",
			cf["type"].dump().c_str());
		abort();
	}
}

bool CF::is_ready_exec(const json &fp, const json &cf, const Context &ctx)
{
	for (auto i=0u; i<cf["args"].size(); i++) {
		auto arg=cf["args"][i];
		auto param_spec=fp[cf["code"].get<std::string>()]["args"][i];
		bool is_input=param_spec["type"]!="name";
		if (is_input) {
			if (!ctx.can_eval(arg)) { return false; }
		} else {
			if (!ctx.can_eval_ref(arg["ref"])) { return false; }
		}
	}
	return true;
}

