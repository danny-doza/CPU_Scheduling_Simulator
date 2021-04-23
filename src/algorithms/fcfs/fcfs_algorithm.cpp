#include "algorithms/fcfs/fcfs_algorithm.hpp"

#include <cassert>
#include <stdexcept>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"

/*
    Here is where you should define the logic for the FCFS algorithm.
*/

FCFSScheduler::FCFSScheduler(int slice) {
    if (slice != -1) {
        throw("FCFS must have a timeslice of -1");
    }
}

std::shared_ptr<SchedulingDecision> FCFSScheduler::get_next_thread() {
	/* TODO: FIX EXPLANATION */
    
    std::shared_ptr<Thread> temp_thread = nullptr;
    std::string explanation = "";
    if (!threads.empty()) {
        temp_thread = threads.front();
    	if (temp_thread->bursts.empty()) {
    	    threads.pop();
    	    temp_thread = threads.front();
    	} else {
    	    if (temp_thread->current_state == ThreadState::BLOCKED) {
    	        threads.pop();
    	        temp_thread = threads.front();
    	    }
    	}
    } else {
    	temp_thread = nullptr;
    }
    explanation = "Selected from x processes. Running to completion of burst.";

    return std::make_shared<SchedulingDecision>(temp_thread, explanation, -1);
}

void FCFSScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
    threads.push(thread);
}

size_t FCFSScheduler::size() const {
    return threads.size();
}
