/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_101258619_101166589.hpp>

const unsigned int TIME_QUANTUM = 100; // Time quantum for Round Robin Scheduling

void EP(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.priority < second.priority); // Pick the first element with the highest priority
                } 
            );
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

    unsigned int time_elapsed_in_quantum = 0; //Track time used in current quantum

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);

                process.state = READY;  //Set the process state to READY
                ready_queue.push_back(process); //Add the process to the ready queue
                job_list.push_back(process); //Add it to the list of processes

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);

                // Check for preemption if new process has higher priority
                if(running.PID != -1 && process.priority < running.priority) {
                    // Preempt the running process
                    running.state = READY;
                    execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
                    
                    ready_queue.push_back(running); //Add back to ready queue
                    sync_queue(job_list, running); //Sync the job list

                    idle_CPU(running); //Set CPU to idle
                    
                    EP(ready_queue); // Sort by priority
                    running = ready_queue.front(); // Get the highest priority process
                    ready_queue.erase(ready_queue.begin()); // Remove it from ready queue
                    running.state = RUNNING; // Set its state to RUNNING
                    sync_queue(job_list, running); //Sync the job list
                    execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
                    time_elapsed_in_quantum = 0; // Reset quantum timer
                }
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue

        for (int i = 0; i < wait_queue.size(); i++)
        {
            wait_queue[i].io_time += 1; // Increment I/O time for each process in the wait queue

            if (wait_queue[i].io_time > wait_queue[i].io_duration) {
                // Move process to ready queue
                wait_queue[i].state = READY; 
                wait_queue[i].io_time = 0;
                execution_status += print_exec_status(current_time, wait_queue[i].PID, WAITING, READY);
                ready_queue.push_back(wait_queue[i]);
                sync_queue(job_list, wait_queue[i]);
                wait_queue.erase(wait_queue.begin() + i);
                i--; // Adjust index after erasing an element
            } else {
                sync_queue(job_list, wait_queue[i]);
            }
        }

        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        // External Priority Scheduling with Round Robin
        // If CPU is idle and there are processes in the ready queue, run the next process
        if (running.PID == -1 && !ready_queue.empty()) {
            EP(ready_queue); // Sort by priority

            running = ready_queue.front();
            ready_queue.erase(ready_queue.begin());
            running.state = RUNNING;
            sync_queue(job_list, running);
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
            time_elapsed_in_quantum = 0;
        }

        // If a process is running, decrement its remaining time
        if (running.PID != -1) {

            running.remaining_time -= 1;
            time_elapsed_in_quantum += 1;

            unsigned int cpu_used = running.processing_time - running.remaining_time; // Calculate CPU time used

            if (running.remaining_time == 0) {
                // Terminate process
                terminate_process(running, job_list);
                execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, TERMINATED);

                idle_CPU(running); // Set CPU to idle
                time_elapsed_in_quantum = 0;
            }

            // Check for I/O interrupt
            else if (running.io_freq > 0 && cpu_used % running.io_freq == 0) {
                // Move process to wait queue
                running.state = WAITING;
                execution_status += print_exec_status(current_time + 1, running.PID, RUNNING, WAITING);
                
                PCB temp = running; // Create a temporary copy of the running process
                temp.io_time = 0; // Reset I/O time for the wait queue
                wait_queue.push_back(temp);
                sync_queue(job_list, temp);

                idle_CPU(running); // Set CPU to idle
                time_elapsed_in_quantum = 0;
            }

            // Check if time quantum has expired
            else if (time_elapsed_in_quantum >= TIME_QUANTUM && !ready_queue.empty()) {
                // Preempt the running process
                running.state = READY;
                execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
                
                ready_queue.push_back(running); //Add back to ready queue
                sync_queue(job_list, running); //Sync the job list

                idle_CPU(running); //Set CPU to idle
                time_elapsed_in_quantum = 0;
            }
        }

        current_time += 1;
        /////////////////////////////////////////////////////////////////

    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}