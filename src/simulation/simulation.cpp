#include <fstream>
#include <iostream>

#include "algorithms/fcfs/fcfs_algorithm.hpp"
#include "algorithms/rr/rr_algorithm.hpp"

#include "simulation/simulation.hpp"
#include "types/enums.hpp"

#include "utilities/flags/flags.hpp"

Simulation::Simulation(FlagOptions flags)
{
    // Hello!
    if (flags.scheduler == "FCFS")
    {
        // Create a FCFS scheduling algorithm
        this->scheduler = std::make_shared<FCFSScheduler>();
    }
    else if (flags.scheduler == "RR")
    {
        // Create a RR scheduling algorithm
        this->scheduler = std::make_shared<RRScheduler>(flags.time_slice);
    }
    this->flags = flags;
    this->logger = Logger(flags.verbose, flags.per_thread, flags.metrics);
}

void Simulation::run()
{
    this->read_file(this->flags.filename);

    while (!this->events.empty())
    {
        auto event = this->events.top();
        this->events.pop();

        // Invoke the appropriate method in the simulation for the given event type.

        switch (event->type)
        {
        case PROCESS_ARRIVED:
            this->handle_process_arrived(event);
            break;

        case PROCESS_DISPATCH_COMPLETED:
            this->handle_dispatch_completed(event);
            break;

        case CPU_BURST_COMPLETED:
            this->handle_cpu_burst_completed(event);
            break;

        case IO_BURST_COMPLETED:
            this->handle_io_burst_completed(event);
            break;
        case PROCESS_COMPLETED:
            this->handle_process_completed(event);
            break;

        case PROCESS_PREEMPTED:
            this->handle_process_preempted(event);
            break;

        case DISPATCHER_INVOKED:
            this->handle_dispatcher_invoked(event);
            break;
        }

        // If this event triggered a state change, print it out.
        if (event->thread && event->thread->current_state != event->thread->prev_state)
        {
            this->logger.print_state_transition(event, event->thread->prev_state, event->thread->current_state);
        }
        this->system_stats.total_time = event->time;
        event.reset();
    }
    // We are done!

    std::cout << "SIMULATION COMPLETED!\n\n";

    for (auto entry : this->processes)
    {
        this->logger.print_per_thread_metrics(entry.second);
    }

    logger.print_simulation_metrics(this->calculate_statistics());
}

//==============================================================================
// Event-handling methods
//==============================================================================

void Simulation::handle_process_arrived(const std::shared_ptr<Event> event)
{
    event->thread->set_state(ThreadState::READY, event->time);
    this->scheduler->add_to_ready_queue(event->thread);
    if (this->active_thread == nullptr) {
	add_event(std::make_shared<Event>(EventType::DISPATCHER_INVOKED, event->time,event->event_num++, 
						event->thread, event->scheduling_decision));
    } else {
    	add_event(std::make_shared<Event>(EventType::PROCESS_PREEMPTED, event->time,event->event_num++, 
						event->thread, event->scheduling_decision));
    }
	return;
}

void Simulation::handle_dispatch_completed(const std::shared_ptr<Event> event)
{

    event->thread->set_state(RUNNING, event->time);
    event->thread->start_time = event->time;
    this->prev_thread = this->active_thread;

    EventType temp_event_type = PROCESS_ARRIVED;
    
    auto temp_burst = this->active_thread->pop_next_burst(CPU);
    if (this->scheduler->time_slice == -1) {
    	if (this->active_thread->bursts.empty()) {
    	    temp_event_type = PROCESS_COMPLETED;
    	} else {
    	    temp_event_type = CPU_BURST_COMPLETED;
    	}
    	event->thread->service_time += temp_burst->length;
    	this->system_stats.service_time += temp_burst->length;
    } else {
	if (this->scheduler->time_slice < temp_burst->length) {
	    	temp_event_type = PROCESS_PREEMPTED;
	} else {
    	    if (this->active_thread->bursts.empty()) {
    	        temp_event_type = PROCESS_COMPLETED;
    	    } else {
    	        temp_event_type = CPU_BURST_COMPLETED;
    	    }
    	    event->thread->service_time += temp_burst->length;
    	    this->system_stats.service_time += temp_burst->length;
    	}
    }
    
    add_event(std::make_shared<Event>(temp_event_type, event->time + temp_burst->length, event->event_num++, event->thread,
    					event->scheduling_decision));
    return;
}

void Simulation::handle_cpu_burst_completed(const std::shared_ptr<Event> event)
{
    event->thread->set_state(BLOCKED, event->time);
    
    add_event(std::make_shared<Event>(EventType::IO_BURST_COMPLETED, event->time, event->event_num++, event->thread,
    					event->scheduling_decision));
}

void Simulation::handle_io_burst_completed(const std::shared_ptr<Event> event)
{
    event->thread->set_state(READY, event->time);
    this->scheduler->add_to_ready_queue(event->thread);
    
    event->thread->io_time += this->active_thread->bursts.front()->length;
    
    this->active_thread->pop_next_burst(event->thread->bursts.front()->burst_type);
    add_event(std::make_shared<Event>(EventType::DISPATCHER_INVOKED, event->time, event->event_num++, 
    					event->thread, event->scheduling_decision));
}

void Simulation::handle_process_completed(const std::shared_ptr<Event> event)
{
    event->thread->set_state(EXIT, event->time);
    event->thread->end_time = event->time;

    this->system_stats.thread_counts[grab_correct_index(this->active_thread->priority)]++;
    this->system_stats.avg_thread_response_times[grab_correct_index(this->active_thread->priority)] += 
    	this->active_thread->response_time();
    this->system_stats.avg_thread_turnaround_times[grab_correct_index(this->active_thread->priority)] += 
	this->active_thread->turnaround_time();
    this->prev_thread = this->active_thread;
    this->active_thread = nullptr;
    
    if (this->scheduler->get_next_thread() != nullptr) {
    	add_event(std::make_shared<Event>(EventType::DISPATCHER_INVOKED, event->time, event->event_num++, event->thread, 
    						event->scheduling_decision));
    }
    
    return;
}

void Simulation::handle_process_preempted(const std::shared_ptr<Event> event)
{
    event->thread->set_state(READY, event->time);
    this->scheduler->add_to_ready_queue(event->thread);
    if (this->scheduler->time_slice != -1) {
    	this->active_thread->bursts.front()->update_time(this->scheduler->time_slice);
    }
    
    if (this->active_thread == nullptr) {
	add_event(std::make_shared<Event>(EventType::DISPATCHER_INVOKED, event->time,event->event_num++, 
						event->thread, event->scheduling_decision));
    } else {

	add_event(std::make_shared<Event>(EventType::DISPATCHER_INVOKED, event->time,event->event_num++, 
						event->thread, event->scheduling_decision));
    }
    return;
}

void Simulation::handle_dispatcher_invoked(const std::shared_ptr<Event> event)
{
    if (this->active_thread != nullptr) {
    	this->prev_thread = this->active_thread;
    }
    
    EventType temp_event_type = EventType::PROCESS_ARRIVED;
    
    auto temp_scheduling_decision = this->scheduler->get_next_thread();
    if (temp_scheduling_decision == nullptr) {
        this->active_thread = nullptr;
    } else {
        this->active_thread = event->thread;
        if (this->prev_thread != nullptr) {
	    if (this->active_thread->process_id == this->prev_thread->process_id) {
		    temp_event_type = EventType::THREAD_DISPATCH_COMPLETED;
	    } else {
		    temp_event_type = EventType::PROCESS_DISPATCH_COMPLETED;
	    }
        } else {
            temp_event_type = EventType::PROCESS_DISPATCH_COMPLETED;
        }
        auto temp_burst = this->active_thread->get_next_burst(CPU);
        add_event(std::make_shared<Event>(temp_event_type, event->time + temp_burst->length,
						event->event_num++, temp_scheduling_decision->thread, temp_scheduling_decision));
    }
    
    if (this->active_thread == nullptr) {
    	while(true) {
    		continue;
    	}
    }
    
    return;
}

//==============================================================================
// Utility methods
//==============================================================================

SystemStats Simulation::calculate_statistics()
{
    for (int i = 0; i < 4; i++) {
    	if (this->system_stats.thread_counts[i] != 0) {
    	    this->system_stats.avg_thread_response_times[i] = 
    		this->system_stats.avg_thread_response_times[i]/this->system_stats.thread_counts[i];
    	    this->system_stats.avg_thread_turnaround_times[i] = 
    		this->system_stats.avg_thread_response_times[i]/this->system_stats.thread_counts[i];
    	}
    }
    
    return this->system_stats;
}

int Simulation::grab_correct_index(ProcessPriority priority) {
	int index = -1;
	switch(priority) {
		case SYSTEM:
			index = 0;
			break;
		case INTERACTIVE:
			index = 1;
			break;
		case NORMAL:
			index = 2;
			break;
		case BATCH:
			index = 3;
			break;
		default:
			break;
	}
	return index;
}

void Simulation::add_event(std::shared_ptr<Event> event)
{
    if (event != nullptr)
    {
        this->events.push(event);
    }
}

void Simulation::read_file(const std::string filename)
{
    std::ifstream input_file(filename.c_str());

    if (!input_file)
    {
        std::cerr << "Unable to open simulation file: " << filename << std::endl;
        throw(std::logic_error("Bad file."));
    }

    int num_processes;
    int thread_switch_overhead; // This is discarded for this semester

    input_file >> num_processes >> thread_switch_overhead >> this->process_switch_overhead;

    for (int proc = 0; proc < num_processes; ++proc)
    {
        auto process = read_process(input_file);

        this->processes[process->process_id] = process;
    }
}

std::shared_ptr<Process> Simulation::read_process(std::istream &input)
{
    int process_id, priority;
    int num_threads;

    input >> process_id >> priority >> num_threads;

    auto process = std::make_shared<Process>(process_id, (ProcessPriority)priority);

    // iterate over the threads
    for (int thread_id = 0; thread_id < num_threads; ++thread_id)
    {
        process->threads.emplace_back(read_thread(input, thread_id, process_id, (ProcessPriority)priority));
    }

    return process;
}

std::shared_ptr<Thread> Simulation::read_thread(std::istream &input, int thread_id, int process_id, ProcessPriority priority)
{   
    // Stuff
    int arrival_time;
    int num_cpu_bursts;

    input >> arrival_time >> num_cpu_bursts;

    auto thread = std::make_shared<Thread>(arrival_time, thread_id, process_id, priority);

    for (int n = 0, burst_length; n < num_cpu_bursts * 2 - 1; ++n)
    {
        input >> burst_length;

        BurstType burst_type = (n % 2 == 0) ? BurstType::CPU : BurstType::IO;

        thread->bursts.push(std::make_shared<Burst>(burst_type, burst_length));
    }

    this->events.push(std::make_shared<Event>(EventType::PROCESS_ARRIVED, thread->arrival_time, this->event_num, thread, nullptr));
    this->event_num++;

    return thread;
}
