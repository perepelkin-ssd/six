#include "fp_json.h"


#include "common.h"
#include "jfp.h"
#include "json.h"
#include "remote_monitor.h"

ExecJsonFp::ExecJsonFp(const std::string &json_content, Factory &fact,
		const std::string &main_arg)
	: fact_(fact), json_dump_(json::parse(json_content).dump()),
		main_arg_(main_arg)
{}

ExecJsonFp::ExecJsonFp(BufferPtr &buf, Factory &fact)
	: fact_(fact)
{
	json_dump_=Buffer::popString(buf);
}

void ExecJsonFp::run(const EnvironPtr &env)
{
	Id fp_id=env->create_id("_fp_");

	json j=json::parse(json_dump_);

	Buffers bufs;
	bufs.push_back(Buffer::create(STAG_JfpReg));
	fp_id.serialize(bufs);
	bufs.push_back(Buffer::create(j.dump()));

	std::string opt_arg="";

	auto main_sub=j["main"];
	if (main_sub["args"].size()!=0) {
		assert(main_sub["args"].size()==1);
		assert(main_sub["args"][0]["type"]=="string");
		opt_arg="{\"type\": \"sconst\", \"value\": \"" + main_arg_
			+ "\"}";
	}

	RPtr rptr(env->comm().get_rank(), create_counter(env->comm().get_size(),
		[env, fp_id, this, opt_arg]() {
			env->submit(TaskPtr(new JfpExec(fp_id, env->create_id("_main"),
			"{\"type\": \"exec\", \"code\": \"main\", \"args\": ["
			+ opt_arg +
			"]}", LocatorPtr(new CyclicLocator(0)),
			fact_)));
		}
	));

	rptr.serialize(bufs);
	env->send_all(bufs);
}

void ExecJsonFp::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_ExecJsonFp));
	bufs.push_back(Buffer::create(json_dump_));
}

std::string ExecJsonFp::to_string() const
{
	return "(json_fp)";
}

