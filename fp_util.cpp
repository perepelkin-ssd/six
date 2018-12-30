#include "fp_util.h"

#include "common.h"

extern "C" {
#include "bi.h"
#include "expr.h"
#include "fp.h"
#include "ref.h"
#include "rule.h"
#include "rules.h"
#include "sub.h"
}

std::set<Id> CF::get_requested_dfs(const void *cf, const Context &ctx)
{
	std::set<Id> res;

	const void *rules=bi_rules(cf);
	if (rules_count(rules)==0) { return {}; }

	if (bi_type(cf)==BI_WHILE) {
		const void *start=bi_start(cf);
		if (ctx.can_eval(start)) {
			Context ctx1=ctx;
			ctx1.set_param(bi_var(cf), ctx.eval(start), true);
			auto rules=bi_rules(cf);
			auto rls_count=rules_count(cf);
			for (auto i=0u; i<rls_count; i++) {
				auto rule=rules_rule(rules, i);
				if (rule_type(rule)==RULE_ENUM
						&& rule_property(rule)==PROPERTY_REQUEST) {
					auto count=rule_refs_count(rule);
					for (auto j=0u; j<count; j++) {
						auto ref=rule_ref(rule, j);
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
		auto count=rules_count(rules);
		for (auto i=0u; i<count; i++) {
			const void *rule=rules_rule(rules, i);
			if (rule_type(rule)==RULE_ENUM &&
					rule_property(rule)==PROPERTY_REQUEST) {
				auto refs_count=rule_refs_count(rule);
				for (auto j=0u; j<refs_count; j++) {
					const void *ref=rule_ref(rule, j);
					if (ctx.can_eval_ref(ref)) {
						res.insert(ctx.eval_ref(ref));
					}
				}
			}
		}
	}
	return res;
}

bool CF::has_pushes(const void *cf)
{
	const void *rules=bi_rules(cf);
	auto count=rules_count(rules);
	if (count==0) { return {}; }
	for (auto i=0u; i<count; i++) {
		const void *rule=rules_rule(rules, i);
		if (rule_type(rule)==RULE_FLAGS) {
			auto flags_count=rule_flags_count(rule);
			for (auto fc=0; fc<flags_count; fc++) {
				if (rule_flag(rule, fc)==FLAG_HAS_PUSHES) {
					return true;
				}
			}
		}
	}
	return false;
}

std::set<std::pair<Id, Id> > CF::get_afterpushes(const void *cf,
	const Context &ctx)
{
	auto rules=bi_rules(cf);
	auto rls_count=rules_count(rules);
	if (rls_count==0) { return {}; }

	std::set<std::pair<Id, Id> > res;

	for (auto i=0u; i<rls_count; i++) {
		const void *rule=rules_rule(rules, i);
		if (rule_type(rule)==RULE_MAP
				&& rule_property(rule)==PROPERTY_AFTERPUSH) {
			const void *expr=rule_value(rule);
			assert(expr_type(expr)==EXPR_ID);
			const void *ref=expr_id_value(expr);
			res.insert(std::make_pair(
				ctx.eval_ref(rule_key(rule)),
				ctx.eval_ref(ref)
			));
		}
	}
	return res;
}

bool CF::is_df_requested(const void *cf, const Context &ctx,
	const Id &dfid)
{
	auto rules=bi_rules(cf);
	auto rls_count=rules_count(rules);
	if (rls_count==0) { return false; }

	for (auto i=0u; i<rls_count; i++) {
		const void *rule=rules_rule(rules, i);
		if (rule_type(rule)==RULE_ASSIGN
				&& rule_property(rule)==PROPERTY_REQ_COUNT
				&& ctx.eval_ref(rule_key(rule))==dfid) {
			auto count=(int)(*ctx.eval(rule_value(rule)));
			assert(count>0);
			return count>0;
		}
	}
	return false || is_requested_unlimited(cf, ctx, dfid);
}

bool CF::is_requested_unlimited(const void *cf, const Context &ctx,
	const Id &dfid)
{
	auto rules=bi_rules(cf);
	auto rls_count=rules_count(rules);
	if (rls_count==0) {
		bi_print(nullptr, cf);
		ABORT("no rules");
	}

	for (auto i=0u; i<rls_count; i++) {
		const void *rule=rules_rule(rules, i);
		if (rule_type(rule)==RULE_ENUM
				&& rule_property(rule)==PROPERTY_REQ_UNLIMITED) {
			auto refs_count=rule_refs_count(rule);
			for (auto j=0u; j<refs_count; j++) {
				if (ctx.eval_ref(rule_ref(rule, j))==dfid) {
					return true;
				}
			}
		}
	}
	return false;
}

size_t CF::get_requests_count(const void *cf, const Context &ctx,
	const Id &dfid)
{
	auto rules=bi_rules(cf);
	auto rls_count=rules_count(rules);
	if (rls_count==0) {
		bi_print(nullptr, cf);
		ABORT("no rules");
	}

	for (auto i=0u; i<rls_count; i++) {
		const void *rule=rules_rule(rules, i);
		if (rule_type(rule)==RULE_ASSIGN
				&& rule_property(rule)==PROPERTY_REQ_COUNT
				&& ctx.eval_ref(rule_key(rule))==dfid) {
			auto count=(int)(*ctx.eval(rule_value(rule)));
			assert(count>0);
			return count;
		}
	}
	ABORT("no req_count for: " + dfid.to_string());
}

bool CF::is_ready(const void *fp, const void *cf, const Context &ctx)
{
	switch (bi_type(cf)) {
		case BI_EXEC:
			return CF::is_ready_exec(fp, cf, ctx);
		case BI_IF:
			return ctx.can_eval(bi_cond(cf));
		case BI_FOR:
			return ctx.can_eval(bi_first(cf))
				&& ctx.can_eval(bi_last(cf));
		case BI_WHILE:
			{
				if (!ctx.can_eval(bi_start(cf))
						|| !ctx.can_eval_ref(bi_wout(cf))) {
					return false;
				}
				Context ctx1=ctx;
				ctx1.set_param(bi_var(cf), ctx.eval(bi_start(cf)));
				return ctx1.can_eval(bi_cond(cf));
			}
		default:
			ABORT("Unsupported type: " + std::to_string(bi_type(cf)));
	}
}

bool CF::is_ready_exec(const void *fp, const void *cf, const Context &ctx)
{
	auto sub=fp_sub(fp, bi_sub_idx(cf));
	auto args_count=bi_args_count(cf);
	assert(sub_params_count(sub)==args_count);
	for (auto i=0u; i<args_count; i++) {
		auto arg=bi_arg(cf, i);
		auto param=sub_partype(sub, i);
		bool is_input=param!=PARAM_NAME;;
		if (is_input) {
			if (!ctx.can_eval(arg)) { return false; }
		} else {
			if (!ctx.can_eval_ref(expr_id_value(arg))) { return false; }
		}
	}
	return true;
}


std::set<Id> CF::get_afterdels(const void *cf, const Context &ctx)
{
	auto rules=bi_rules(cf);
	auto rls_count=rules_count(rules);
	if (rls_count==0) { return {}; }

	std::set<Id> res;
	for (auto i=0u; i<rls_count; i++) {
		const void *rule=rules_rule(rules, i);
		if (rule_type(rule)==RULE_INDEXED) {
			auto refs_count=rule_refs_count(rule);
			for (auto j=0u; j<refs_count; j++) {
				const void *ref=rule_ref(rule, j);
				res.insert(ctx.eval_ref(ref));
			}
		} else if (rule_type(rule)==RULE_ENUM
				&& rule_property(rule)==PROPERTY_DELETE) {
			auto refs_count=rule_refs_count(rule);
			for (auto j=0u; j<refs_count; j++) {
				const void *ref=rule_ref(rule, j);
				res.insert(ctx.eval_ref(ref));
			}
		}
	}
	return res;
}


std::set<Id> CF::get_aftersets(const void *cf, const Context &ctx)
{
	auto rules=bi_rules(cf);
	auto rls_count=rules_count(rules);
	if (rls_count==0) { return {}; }

	std::set<Id> res;
	for (auto i=0u; i<rls_count; i++) {
		const void *rule=rules_rule(rules, i);
		if (rule_type(rule)==RULE_INDEXED_SETDFS) {
			auto refs_count=rule_refs_count(rule);
			for (auto j=0u; j<refs_count; j++) {
				const void *ref=rule_ref(rule, j);
				res.insert(ctx.eval_ref(ref));
			}
		} else if (rule_type(rule)==RULE_ENUM
				&& rule_property(rule)==PROPERTY_SET) {
			auto refs_count=rule_refs_count(rule);
			for (auto j=0u; j<refs_count; j++) {
				const void *ref=rule_ref(rule, j);
				res.insert(ctx.eval_ref(ref));
			}
		}
	}
	return res;
}

LocatorPtr CF::get_locator(const void *cf, const LocatorPtr &base,
	const Context &ctx)
{
	const void *rules=bi_rules(cf);
	auto count=rules_count(rules);
	if (count==0) { return LocatorPtr(nullptr); }

	std::set<std::pair<Id, Id> > res;

	for (auto i=0u; i<count; i++) {
		const void *rule=rules_rule(rules, i);
		if (rule_type(rule)==RULE_EXPR
				&& rule_property(rule)==PROPERTY_LOCATOR_CYCLIC) {
			if (bi_type(cf)==BI_WHILE) {
				Context ctx1=ctx;
				auto var=bi_var(cf);
				if (!ctx.can_eval(bi_start(cf))) {
					ABORT("Cannot evaluate start expression for locator");
				}
				auto val=ctx.eval(bi_start(cf));
				ctx1.set_param(var, val, true);
				if (!ctx1.can_eval(rule_expr(rule))) {
					ABORT("Cannot evaluate locator expression");
				}
				return LocatorPtr(new CyclicLocator(
					(int)(*ctx1.eval(rule_expr(rule)))));
			} else {
				return LocatorPtr(new CyclicLocator(
					(int)(*ctx.eval(rule_expr(rule)))));
			}
		}
	}

	return LocatorPtr(nullptr);
}

std::map<Id, const void *> CF::get_global_locators(const void *rules,
	const Context &ctx)
{
	auto count=rules_count(rules);
	if (count==0) { return {}; }

	std::map<Id, const void *> res;

	for (auto i=0u; i<count; i++) {
		const void *rule=rules_rule(rules, i);
		if (rule_type(rule)==RULE_MAP
				&& rule_property(rule)==PROPERTY_LOCATOR_CYCLIC) {
			const void *ref=rule_key(rule);
			auto prefix=ctx.eval_ref(ref, 0);
			if(res.find(prefix)!=res.end()) {
				ref_print(ref);
				ABORT("Locator redefinition");
			}
			res[prefix]=(rule);
		}
	}

	return res;
}

LocatorPtr CF::get_global_locator(const void *rule,
	const std::vector<int> &indices, const Context &ctx)
{
	switch (rule_type(rule)) {
		case RULE_MAP:
			if (rule_property(rule)==PROPERTY_LOCATOR_CYCLIC) {
				// Algorithm:
				// compare indices count
				// make context' with vars assigned
				Context ctx1=ctx;
				const void *ref=rule_key(rule);
				if(indices.size()!=ref_idx_count(ref)) {
					rule_print(rule);
					ABORT("Idx length missmatch");
				}
				for (auto i=0u; i<indices.size(); i++) {
					auto idx=ref_idx(ref, i);
					assert(expr_type(idx)==EXPR_ID
						&& ref_idx_count(expr_id_value(idx))==0);
					Name var(ref_basename(expr_id_value(idx)));
					ctx1.set_param(var, IntValue::create(indices[i]), true);
				}
				return LocatorPtr(new CyclicLocator(
					(int)(*ctx1.eval(rule_value(rule)))));
			} else {
				rule_print(rule);
				ABORT("Unsupported locator type: " 
					+ std::to_string(rule_property(rule)));
			}
		case RULE_EXPR:
			if (rule_property(rule)==PROPERTY_LOCATOR_CYCLIC) {
				return LocatorPtr(new CyclicLocator(
					(int)(*ctx.eval(rule_expr(rule)))));
			} else {
				rule_print(rule);
				ABORT("Unsupported locator type: " 
					+ std::to_string(rule_property(rule)));
			}
		default:
			rule_print(rule);
			ABORT("Rule type not supported: " 
				+ std::to_string(rule_type(rule)));
	}
}

const void *CF::get_loc_spec(const void *cf)
{
	const void *rules=bi_rules(cf);
	auto count=rules_count(rules);

	if (count==0) {
		bi_print(nullptr, cf);
		ABORT("No locator");
	}

	if (bi_type(cf)!=BI_EXEC) {
		bi_print(nullptr, cf);
		ABORT("No id (not 'exec')");
	}

	auto ref=bi_id(cf);

	for (auto i=0u; i<count; i++) {
		auto rule=rules_rule(rules, i);
		if (rule_type(rule)==RULE_EXPR
				&& rule_property(rule)==PROPERTY_LOCATOR_CYCLIC) {
			return rule;
		}
	}
	
	bi_print(nullptr, cf);
	ABORT("No locator");
}

bool CFFor::is_unroll_at_once(const void *cf)
{
	assert(bi_type(cf)==BI_FOR || bi_type(cf)==BI_WHILE);

	const void *rules=bi_rules(cf);
	auto count=rules_count(rules);

	if (count==0) {
		ABORT("Not unroll-at-once strategy?");
		// return true by default?
	}
	
	for (auto i=0u; i<count; i++) {
		auto rule=rules_rule(rules, i);
		if (rule_type(rule)!=RULE_FLAGS) { continue; }

		auto flags_count=rule_flags_count(rule);
		for (auto j=0u; j<flags_count; j++) {
			auto flag=rule_flag(rule, j);
			if (flag==FLAG_UNROLL_AT_ONCE) {
				return true;
			}
		}
	}
	return false;
}
