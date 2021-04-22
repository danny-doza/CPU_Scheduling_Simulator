#include <cassert>
#include <cstddef>
#include <stdexcept>
#include "types/thread/thread.hpp"

void Thread::set_ready(int time) {	
	if (current_state != ThreadState::EXIT) {
	    	prev_state = current_state;
		current_state = ThreadState::READY;
	} else {
		fprintf(stderr, "Invalid READY state change");
		_Exit(EXIT_FAILURE);
	}
}

void Thread::set_running(int time) {
	if ((current_state != ThreadState::NEW) && (current_state != ThreadState::BLOCKED) && (current_state != ThreadState::EXIT)) {
	    	prev_state = current_state;
  		current_state = ThreadState::RUNNING;
	} else {
		fprintf(stderr, "Invalid RUNNING state change (from %d)", current_state);
		_Exit(EXIT_FAILURE);
	}
}

void Thread::set_blocked(int time) {
	if (current_state == ThreadState::RUNNING) {
	    	prev_state = current_state;
  		current_state = ThreadState::RUNNING;
	} else {
		fprintf(stderr, "Invalid BLOCKED state change.");
		_Exit(EXIT_FAILURE);
	}
}

void Thread::set_finished(int time) {
    	if (current_state == ThreadState::RUNNING) {
    		prev_state = current_state;
    		current_state = ThreadState::EXIT;
    	} else {
    		fprintf(stderr, "Invalid EXIT state change.");
    		_Exit(EXIT_FAILURE);
    	}
}

int Thread::response_time() const {
    // TODO
    return start_time - arrival_time;
}

int Thread::turnaround_time() const {
    return end_time - arrival_time;
}

void Thread::set_state(ThreadState state, int time) {
    state_change_time = time;
    switch (state) {
    	case ThreadState::NEW: {
    	    // TODO: File has arrived
    	    break;
    	} case ThreadState::READY: {
    	    set_ready(time);
    	    break;
    	} case ThreadState::RUNNING: {
    	    set_running(time);
    	    break;
    	} case ThreadState::BLOCKED: {
    	    set_blocked(time);
    	    break;
    	} case ThreadState::EXIT: {
    	    set_finished(time);
    	    break;
    	} default: {
    	    fprintf(stderr, "Unrecognized enum type.");
    	    break;
    	}
    }
}

std::shared_ptr<Burst> Thread::get_next_burst(BurstType type) {
    return bursts.front();
}

std::shared_ptr<Burst> Thread::pop_next_burst(BurstType type) {
    std::shared_ptr<Burst> temp_burst = bursts.front();
    bursts.pop();
    return temp_burst;
}
