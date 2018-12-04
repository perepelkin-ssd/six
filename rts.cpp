#include "rts.h"

#include "common.h"

RTS::~RTS()
{
	delete stopper_;
}

RTS::RTS(Comm &comm)
	: comm_(comm), env_(this), idle_flag_(false)
{
	stopper_=new IdleStopper<NodeId>(
		comm_.get_rank(),
		[this](const NodeId &id) {
			NodeId next_rank((comm_.get_rank()+1)%comm_.get_size());
			comm_.send(next_rank, TAG_IDLE_STOPPER, &id, sizeof(id));
		},
		[this](){
			comm_.send_all(TAG_IDLE, nullptr, 0);
		}
	);

	comm_.set_handler([this](const NodeId &src, const Tag &tag,
			const void *buf, size_t size) {
		on_message(src, tag, buf, size);
	});
}

CfEnv &RTS::get_env() { return env_; }

void RTS::wait_all(bool is_root_node)
{
	std::unique_lock<std::mutex> lk(m_);

	while (!idle_flag_) {
		cv_.wait(lk);
	}
}

void RTS::on_message(const NodeId &src, const Tag &tag, const void *buf,
	size_t size)
{
	printf("%d: on_message %d from %d\n", (int)comm_.get_rank(), tag,
		(int)src);
	switch (tag) {
		case TAG_CF:
			env_.receive_cf(src, buf, size);
			break;
		case TAG_DF:
			//env_.receive_df(src, buf, size);
			break;
		case TAG_DF_VALUE:
			//env_.receive_df_to_cf(src, buf, size);
			break;
		default:
			fprintf(stderr, "%d: RTS::on_message: invalid tag: %d\n",
				(int)comm_.get_rank(), tag);
			abort();
	}
	printf("%d: on_message %d from %d done\n", (int)comm_.get_rank(), tag,
		(int)src);
}

