#ifndef IDLE_STOPPER_H_
#define IDLE_STOPPER_H_

#include <cassert>
#include <functional>
#include <mutex>

#include "printable.h"

// TODO: maybe should call on_all_idle_ on all nodes? m.b. optionally?

/*
	An instace per node is supposed. Each instance is informed once the
	node gets idle or not idle. The instance tracks overall system status
	until all nodes are idle and invokes on_all_idle function.
	Network communications are limited to an int-valued message to the next
	node, which must cause invocation of `receive` method on the destination
	node.
	The message must be send by on_send_next function, assuming there exist
	a hamiltonian loop, i.e. a loop through all nodes with every node
	appearing exatly once.

	Internal algorithm:
	- the message leaves node only in the idle state
	- if the node was non-idle since last visit, the mesage is set to
		current node id
	- when current node id is received for the second time the whole
		system is idle (on_all_idle called and the message is
		eliminated).
*/
template <class NodeIdType>
class IdleStopper : public Printable
{
	NodeIdType current_node_id_;
	std::function<void (const NodeIdType &node_id)> on_send_next_;
	std::function<void ()> on_all_idle_;
	bool marker_here_;
	NodeIdType marker_id_;
	size_t passed_counter_;
	bool dirty_, idle_;
	mutable std::mutex m_;

public:
	// Starts in idle state
	IdleStopper(
			const NodeIdType &current_node_id,
			std::function<void (const NodeIdType &node_id)> on_send_next,
			std::function<void ()> on_all_idle)
		: current_node_id_(current_node_id), on_send_next_(on_send_next),
			on_all_idle_(on_all_idle), marker_here_(false),
			passed_counter_(0), dirty_(false), idle_(true)
	{}

	std::string get_status() const {
		return std::string()+
			(marker_here_? "["+std::to_string(marker_id_)+"]": "[ ]")+
			(dirty_? "D": "d")+
			(idle_? "I": "i")+
			(passed_counter_>0? ">"+std::to_string(passed_counter_): "");

	}
	// generate initial message on current node.
	void start()
	{
		receive(current_node_id_);
	}

	// inject after initial workload is submitted to prevent instant stop
	// on init
	void receive(const NodeIdType &id)
	{
		std::unique_lock<std::mutex> lk(m_);
		assert(!marker_here_);
		marker_here_=true;
		marker_id_=id;

		if (!idle_) {
			marker_id_=current_node_id_;
			passed_counter_=0;
		} else if (dirty_) {
			assert(idle_);
			marker_id_=current_node_id_;
			passed_counter_=0;
			if (idle_) {
				dirty_=false;
				marker_here_=false;
				lk.unlock();
				on_send_next_(marker_id_);
				lk.lock();
			}
		} else if (marker_id_!=current_node_id_) {
			assert(idle_&&!dirty_);
			marker_here_=false;

			lk.unlock();

			on_send_next_(marker_id_);

			lk.lock();
		} else {
			assert(idle_&&!dirty_);
			assert(marker_id_==current_node_id_);
			passed_counter_++;
			if (passed_counter_==3) {
				on_all_idle_();
				marker_here_=false;
				passed_counter_=0;
			} else {
				marker_here_=false;
				on_send_next_(marker_id_);
			}
		}
	}

	// must inform on every idle state change
	void set_idle(bool is_idle)
	{
		std::unique_lock<std::mutex> lk(m_);

		if (!is_idle) {
			dirty_=true;
		}
		idle_=is_idle;

		if (!is_idle || !marker_here_) {
			return;
		}

		assert(marker_id_==current_node_id_);
		assert(passed_counter_==0);

		dirty_=false;
		marker_here_=false;

		lk.unlock();

		on_send_next_(marker_id_);

		lk.lock();
	}

	virtual std::string to_string() const
	{
		std::lock_guard<std::mutex> lk(m_);
		return std::string("Marker: ")
			+ (marker_here_? "here ": "not here ")
			+ (dirty_? "D": "")
			+ (idle_? "I": "");
	}
};

#endif // IDLE_STOPPER_H_
