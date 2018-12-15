#pragma once

#include <memory>

#include "buf_handler.h"
#include "code_lib.h"
#include "comm.h"
#include "df_pusher.h"
#include "df_requester.h"
#include "id.h"
#include "rptr.h"
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

	// Send serialized task to a node (TAG_TASK is front-pushed)
	virtual void send(const NodeId &dest, const Buffers &data)=0;

	// Send serialized task to all nodes (TAG_TASK is front-pushed)
	virtual void send_all(const Buffers &data)=0;

	// add task to local node
	virtual void submit(const TaskPtr &)=0;

	// create system-wide unique id with optional label and indices
	virtual Id create_id(const Name &label="",
		const std::vector<int> &indices={})=0;

	virtual DfPusher &df_pusher()=0;
	
	virtual DfRequester &df_requester()=0;

	virtual ValuePtr get(const Id &key) const=0;
	virtual ValuePtr set(const Id &key, const ValuePtr &val)=0;
	virtual ValuePtr del(const Id &key)=0;

	// Monitor model:
	// when started monitor prevents Environ instance deletion (keeps
	// a smart_pointer) and makes it able to receive messages via
	// remote pointer (PRtr) via BufHandler interface.
	// stop_monitor releases the smart pointer and disables handling.
	virtual RPtr start_monitor(std::function<void(BufferPtr &)>)=0;
	virtual void stop_monitor()=0;

	virtual void exec_extern(const std::string &code,
		std::vector<CodeLib::Argument> &args)=0;
};

typedef std::shared_ptr<Environ> EnvironPtr;
