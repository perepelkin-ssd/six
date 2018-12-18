#include "fp.h"

#include "common.h"

std::set<Id> CF::get_requested_dfs(const json &cf, const Context &ctx)
{
	std::set<Id> res;

	if (cf.find("rules")==cf.end()) { return {}; }

	if (cf["type"]=="while") {
		if (ctx.can_eval(cf["start"])) {
			Context ctx1=ctx;
			ctx1.set_param(cf["var"], ctx.eval(cf["start"]), true);
			for (auto rule : cf["rules"]) {
				assert(rule["type"]=="rule");
				if (rule["ruletype"]=="enum" && rule["property"]
						=="request") {
					for (auto ref : rule["items"]) {
						if (ctx1.can_eval_ref(ref)) {
							res.insert(ctx1.eval_ref(ref));
						}
					}
				}
			}
		} else {
			ABORT("Not supported: start expr in value is not evaluatable yet");
		}
		// Algorithm:
		// If it is while, then try to obtain everything, but be ready,
		// that while variable may be not yet computable
		// In this case reques everything possible, then compute initial
		// parameter, and add it to context to evaluate new requested dfs
	} else {
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
	return false || is_requested_unlimited(cf, ctx, dfid);
}

bool CF::is_requested_unlimited(const json &cf, const Context &ctx,
	const Id &dfid)
{

	if (cf.find("rules")==cf.end()) {
		ABORT("no 'rules' in " + cf.dump(2));
	}
	
	for (auto rule : cf["rules"]) {
		assert(rule["type"]=="rule");
		if (rule["ruletype"]=="enum" && rule["property"]=="req_unlimited") {
			for (auto ref : rule["items"]) {
				if (ctx.eval_ref(ref)==dfid) {
					return true;
				}
			}
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
		if (!ctx.can_eval(cf["start"])
				|| !ctx.can_eval_ref(cf["wout"]["ref"])) {
			return false;
		}
		Context ctx1=ctx;
		ctx1.set_param(cf["var"].get<std::string>(), ctx.eval(cf["start"]));
		return ctx1.can_eval(cf["cond"]);
	} else if (cf["type"]=="if") {
		return ctx.can_eval(cf["cond"]);
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


std::set<Id> CF::get_afterdels(const json &cf, const Context &ctx)
{
	if (cf.find("rules")==cf.end()) { return {}; }

	std::set<Id> res;
	for (auto rule : cf["rules"]) {
		assert(rule["type"]=="rule");
		if (rule["ruletype"]=="indexed") {
			for (auto ref : rule["dfs"]) {
				res.insert(ctx.eval_ref(ref["ref"]));
			}
		} else if (rule["ruletype"]=="enum" && rule["property"]=="delete") {
			for (auto ref : rule["items"]) {
				res.insert(ctx.eval_ref(ref));
			}
		}
	}
	return res;
}

LocatorPtr CF::get_locator(const json &cf, const LocatorPtr &base,
	const Context &ctx)
{
	if (cf.find("rules")==cf.end()) { return LocatorPtr(nullptr); }

	std::set<std::pair<Id, Id> > res;

	for (auto rule : cf["rules"]) {
		assert(rule["type"]=="rule");
		if (rule["ruletype"]=="expr"
				&& rule["property"]=="locator_cyclic") {
			if (cf["type"]=="while") {
				Context ctx1=ctx;
				auto var=cf["var"].get<std::string>();
				if (!ctx.can_eval(cf["start"])) {
					ABORT("Cannot evaluate start expression for locator");
				}
				auto val=ctx.eval(cf["start"]);
				ctx1.set_param(var, val, true);
				if (!ctx1.can_eval(rule["expr"])) {
					ABORT("Cannot evaluate locator expression");
				}
				return LocatorPtr(new CyclicLocator(
					(int)(*ctx1.eval(rule["expr"]))));
			} else {
				return LocatorPtr(new CyclicLocator(
					(int)(*ctx.eval(rule["expr"]))));
			}
		}
	}

	return LocatorPtr(nullptr);
}

std::map<Id, json> CF::get_global_locators(const json &cf,
	const Context &ctx)
{
	if (cf.find("rules")==cf.end()) { return {}; }

	std::map<Id, json> res;

	for (auto rule : cf["rules"]) {
		assert(rule["type"]=="rule");
		if (rule["ruletype"]=="map" && rule["property"]=="locator_cyclic") {
			auto prefix=ctx.eval_ref(json({rule["id"][0]}));
			auto ref=rule["id"];
			auto expr=rule["expr"];
			auto type=rule["property"];
			assert(res.find(prefix)==res.end());
			res[prefix]=json({
				{"ref", ref},
				{"expr", expr},
				{"type", type}
			});
		}
	}

	return res;
}

LocatorPtr CF::get_global_locator(const json &loc_spec,
	const std::vector<int> &indices, const Context &ctx)
{
	if (loc_spec["type"]=="locator_cyclic") {
		// Algorithm:
		// compare indices count
		// make context' with vars assigned
		Context ctx1=ctx;
		if(indices.size()+1!=loc_spec["ref"].size()) {
			ABORT("Idx length missmatch: " +
				loc_spec.dump(2));
		}
		assert(indices.size()+1==loc_spec["ref"].size());
		for (auto i=0u; i<indices.size(); i++) {
			assert(loc_spec["ref"][i+1]["type"]=="id"
				&& loc_spec["ref"][i+1]["ref"].size()==1);
			Name var=loc_spec["ref"][i+1]["ref"][0];
			ctx1.set_param(var, IntValue::create(indices[i]), true);
		}
		return LocatorPtr(new CyclicLocator((int)(*ctx1.eval(loc_spec["expr"]))));
		// TODO improve: make locator factory from json (loc_spec)
	} else {
		fprintf(stderr, "%s\n", loc_spec.dump(2).c_str());
		ABORT("Unsupported locator type: " + loc_spec["type"].dump());
	}
}

json CF::get_loc_spec(const json &cf)
{
	if (cf.find("rules")==cf.end()) {
		ABORT("No locator: " + cf.dump(2));
	}

	if (cf.find("id")==cf.end()) {
		ABORT("No id: " + cf.dump(2));
	}

	auto ref=cf["id"];

	for (auto rule : cf["rules"]) {
		assert(rule["type"]=="rule");
		if (rule["ruletype"]=="expr" 
				&& rule["property"]=="locator_cyclic") {
			json res={
				{"ref", ref},
				{"expr", rule["expr"]},
				{"type", rule["property"]}
			};
			return res;
		}
	}
	
	fprintf(stderr, "rules: %s\n", cf["rules"].dump(2).c_str());
	ABORT("No locator: " + ref.dump(2));
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
