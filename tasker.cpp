#include "tasker.h"

Tasker::Tasker(int fetchers)//need some way of ensuring this is at least 1
{
	//initialise exit clause to false
	exit = false;
	//create number of threads specified
	for (auto i = 0; i < fetchers; ++i) {
		thread_vector.push_back(new thread(bind(&Tasker::Run, this)));
	}

}

Tasker::~Tasker()
{
	{
		//ensure no threads get stuck
		unique_lock<mutex> lock(mx); 
		exit = true;
		run_cv.notify_all();
	}
	
	//join then destroy threads
	for (auto i : thread_vector) {
		i->join();
		delete i;
	}
	//std::cerr << "Killed Threads" << '\n';
}

//let threads know there are no more tasks to add
void Tasker::wait() {
	unique_lock<mutex> lock(mx);
	exit = true;
	end_cv.wait(lock, [&]() -> bool {return end_condition; });
}

//add tasks to task list
void Tasker::add_task(task_ task)
{
	//adding new items no need to exit!
	exit = false;

	unique_lock<mutex> lock(mx);
	//actually add tasks to task list
	task_list.push_back(task);
	//notify run that new task is available
	run_cv.notify_one();
}

void Tasker::Run()
{
	task_ this_task;

	while (true)
	{
		{
			unique_lock<mutex> wait_lock(mx);

			//wait until task list is not empty and exit has been called
			run_cv.wait(wait_lock, [&]() -> bool {return exit || !task_list.empty(); });

			if (exit && task_list.empty()) {
				end_condition = true;
				end_cv.notify_all();
				return;
			}
			//grab next task and remove it from list of tasks
			this_task = task_list.front();
			task_list.pop_front();
		}

		//do this task
		this_task();
	}
}
