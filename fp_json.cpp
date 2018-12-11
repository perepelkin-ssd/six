#include "fp_json.h"

#include "json.hpp" // nlohmann @ github

#include "jfp.h"
#include "remote_monitor.h"

ExecJsonFp::ExecJsonFp(const std::string &json_content, Factory &fact)
	: fact_(fact), json_dump_(nlohmann::json::parse(json_content).dump())
{}

ExecJsonFp::ExecJsonFp(BufferPtr &buf, Factory &fact)
	: fact_(fact)
{
	json_dump_=Buffer::popString(buf);
}

void ExecJsonFp::run(const EnvironPtr &env)
{
	Id fp_id=env->create_id("_fp_");

	nlohmann::json j=nlohmann::json::parse(json_dump_);

	Buffers bufs;
	bufs.push_back(Buffer::create(STAG_JfpReg));
	fp_id.serialize(bufs);
	bufs.push_back(Buffer::create(j.dump()));

	RPtr rptr(env->comm().get_rank(), create_counter(env->comm().get_size(),
		[env, fp_id, this]() {
			env->submit(TaskPtr(new JfpExec(fp_id, env->create_id("_main"),
			"{\"type\": \"exec\", \"code\": \"main\", \"args\": []}",
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

