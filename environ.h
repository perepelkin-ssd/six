#pragma once

#include <memory>

#include "comm.h"
#include "df_pusher.h"
#include "df_requester.h"
#include "id.h"
#include "value.h"

class Task;
typedef std::shared_ptr<Task> TaskPtr;

// Environ implementation MUST provide existence of the corresponding Task
// instance during Environ lifetime
class Environ
{
public:
	virtual ~Environ() {}

	// Get read-only communicator reference
	virtual const Comm &comm()=0;

	// Send serialized task to another node (TAG_TASK is front-pushed)
	virtual void send(const NodeId &dest, const Buffers &data)=0;

	// add task to local node
	virtual void submit(const TaskPtr &)=0;

	// create system-wide unique id with optional label and indices
	virtual Id create_id(const Name &label="",
		const std::vector<int> &indices={})=0;

	virtual DfPusher &df_pusher()=0;
	
	virtual DfRequester &df_requester()=0;
};

typedef std::shared_ptr<Environ> EnvironPtr;
