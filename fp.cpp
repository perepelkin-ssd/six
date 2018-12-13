#include "fp.h"

#include "common.h"

std::set<Id> CF::get_requested_dfs(const json &cf, const Context &ctx)
{
	std::set<Id> res;

	if (cf.find("rules")==cf.end()) { return {}; }

	for (auto rule : cf["rules"]) {
		assert(rule["type"]=="rule");
		if (rule["ruletype"]=="enum" && rule["property"]=="request") {
			for (auto ref : rule["items"]) {
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
		assert(rule["type"]=="rule");
		if (rule["ruletype"]=="flags") {
			for (auto flag : rule["flags"]) {
				if (flag=="has_pushes") {
					return true;
				}
			}
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
		assert(rule["type"]=="rule");
		if (rule["ruletype"]=="map" && rule["property"]=="afterpush") {
			assert(rule["expr"]["type"]=="id");
			res.insert(std::make_pair(
				ctx.eval_ref(rule["id"]),
				ctx.eval_ref(rule["expr"]["ref"])
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
		assert(rule["type"]=="rule");
		if (rule["ruletype"]=="assign" && rule["property"]=="req_count"
				&& ctx.eval_ref(rule["id"])==dfid) {
			auto count=(int)(*ctx.eval(rule["val"]));
			assert(count>0);
			return count>0;
		}
	}
	return false;
}

size_t CF::get_requests_count(const json &cf, const Context &ctx,
	const Id &dfid)
{

	if (cf.find("rules")==cf.end()) {
		ABORT("no 'rules' in " + cf.dump(2));
	}
	
	for (auto rule : cf["rules"]) {
		assert(rule["type"]=="rule");
		if (rule["ruletype"]=="assign" && rule["property"]=="req_count"
				&& ctx.eval_ref(rule["id"])==dfid) {
			auto count=(int)(*ctx.eval(rule["val"]));
			assert(count>0);
			return count;
		}
	}
	fprintf(stderr, "CF::get_requests_count: not available for %s in %s\n",
		dfid.to_string().c_str(), cf.dump(2).c_str());
	ABORT("no req_count for " + dfid.to_string());
}

bool CF::is_ready(const json &fp, const json &cf, const Context &ctx)
{
	if (cf["type"]=="exec") {
		return CF::is_ready_exec(fp, cf, ctx);
	} else if (cf["type"]=="for") {
		return ctx.can_eval(cf["first"]) && ctx.can_eval(cf["last"]);
	} else if (cf["type"]=="while") {
		return ctx.can_eval(cf["first"]) && ctx.can_eval_ref(cf["wout"]);
	} else {
		fprintf(stderr, "ERROR IN %s\n", cf.dump(2).c_str());
		ABORT("Unsupported type: " + cf["type"].dump());
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


bool CFFor::is_unroll_at_once(const json &cf)
{
	if (cf.find("rules")==cf.end()) { return true; }
	
	for (auto rule : cf["rules"]) {
		assert(rule.find("ruletype")!=rule.end());
		if (rule["ruletype"]!="flags") { continue; }
		for (auto flag : rule["flags"]) {
			if (flag=="unroll_at_once") {
				return true;
			}
		}
	}
	return false;
}
