#include "factory.h"

Constructor Factory::set_constructor(STAGS stag, Constructor c)
{
	std::lock_guard<std::mutex> lk(m_);
	printf("set constructor for %d\n", (int)stag);

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

Serializable *Factory::construct(BufferPtr &buf)
{
	STAGS stag=Buffer::pop<STAGS>(buf);

	Constructor cons;
	{
		std::lock_guard<std::mutex> lk(m_);

		if (cons_.find(stag)==cons_.end()) {
			fprintf(stderr, "Factory::construct: illegal stag: %d\n",
				(int)stag);
			abort();
		}

		cons=cons_[stag];
	}
	return cons(buf);
}
 
Constructor Factory::get_constructor(STAGS stag) const
{
	std::lock_guard<std::mutex> lk(m_);

	auto it=cons_.find(stag);

	if (it==cons_.end()) {
		return nullptr;
	} else {
		return it->second;
	}
}
