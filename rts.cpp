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
			comm_.send(next_rank, {
				Buffer::create(TAG_IDLE_STOPPER),
				Buffer::create(id)
			});
		},
		[this](){
			comm_.send_all({
				Buffer::create(TAG_IDLE)
			});
		}
	);

	comm_.set_handler([this](const NodeId &src, const BufferPtr &buf) {
		on_message(src, buf);
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

Constructor RTS::set_constructor(STAGS stag, Constructor c)
{
	std::lock_guard<std::mutex> lk(m_);

	Constructor res=nullptr;

	if (cons_.find(stag)!=cons_.end()) {
		res=cons_[stag];
		if (!c) {
			cons_.erase(stag);
		}
	}

	if(c) {
		cons_[stag]=c;
	}

	return res;
}

void RTS::on_message(const NodeId &src, BufferPtr buf)
{
	TAGS tag=Buffer::pop<RTS::TAGS>(buf);
	printf("%d: on_message tag=%d from %d of size %d\n",
		(int)comm_.get_rank(), (int)tag, (int)src, (int)buf->getSize());
	switch (tag) {
		case TAG_CF:
			env_.receive_cf(src, buf);
			break;
		case TAG_DF:
			env_.receive_df(src, buf);
			break;
		case TAG_DF_VALUE:
			env_.receive_df_to_cf(src, buf);
			break;
		default:
			fprintf(stderr, "%d: RTS::on_message: invalid tag: %d\n",
				(int)comm_.get_rank(), tag);
			abort();
	}
	printf("%d: on_message %d from %d done\n", (int)comm_.get_rank(), tag,
		(int)src);
}

Serializable *RTS::construct(BufferPtr &buf)
{
	STAGS stag=Buffer::pop<STAGS>(buf);

	std::lock_guard<std::mutex> lk(m_);

	if (cons_.find(stag)==cons_.end()) {
		fprintf(stderr, "%d: RTS::construct: illegal stag: %d\n",
			(int)comm_.get_rank(), (int)stag);
		abort();
	}

	return cons_[stag](buf);
}
