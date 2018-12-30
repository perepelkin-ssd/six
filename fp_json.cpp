#include "fp_json.h"


#include "common.h"
#include "jfp.h"
#include "remote_monitor.h"

extern "C" {
#include "fp.h"
#include "sub.h"
}

ExecJsonFp::ExecJsonFp(const BufferPtr &fp_buf, Factory &fact,
		const std::string &main_arg)
	: fact_(fact), fp_buf_(fp_buf), main_arg_(main_arg)
{}

ExecJsonFp::ExecJsonFp(BufferPtr &buf, Factory &fact)
	: fact_(fact)
{
	size_t size=Buffer::pop<size_t>(buf);
	assert(size<=buf->getSize());
	fp_buf_=Buffer::createSubBuffer(buf, 0, size);
	buf=Buffer::createSubBuffer(buf, size);
}

void ExecJsonFp::run(const EnvironPtr &env)
{
	Id fp_id=env->create_id("_fp_");

	Buffers bufs;
	bufs.push_back(Buffer::create(STAG_JfpReg));
	fp_id.serialize(bufs);
	Buffer::pack(bufs, fp_buf_);

	const void *fp=fp_buf_->getData();
	const void *main_sub=fp_sub_by_name(fp, "main");

	if (sub_params_count(main_sub)!=0) {
		assert(sub_params_count(main_sub)==1);
	}

	RPtr rptr(env->comm().get_rank(), create_counter(env->comm().get_size(),
		[env, fp_id, this]() {
			JfpExec::exec_main(env, fp_id, fact_, main_arg_);
		}
	));

	rptr.serialize(bufs);
	env->send_all(bufs);
}

void ExecJsonFp::serialize(Buffers &bufs) const
{
	bufs.push_back(Buffer::create(STAG_ExecJsonFp));
	bufs.push_back(Buffer::create<size_t>(fp_buf_->getSize()));
	bufs.push_back(fp_buf_);
}

std::string ExecJsonFp::to_string() const
{
	return "(bfp of size " + std::to_string(fp_buf_->getSize()) + ")";
}


