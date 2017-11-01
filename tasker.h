#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <list>
#include <mutex>
#include <atomic>


using std::vector;
using std::list;
using std::cout;
using std::endl;
using std::thread;
using std::unique_lock;
using std::mutex;
using std::bind;
using std::condition_variable;
using std::atomic_bool;



class Tasker
{
public:
	typedef std::function<void()> task_;
	//Secure constructor
	static Tasker* create(int);

	//Constructor
	Tasker(int);

	//Destructor
	~Tasker();

	//Add new tasks to task list
	void add_task(task_);

	//have calling thread wait until tasks are completed
	void wait();

private:
	//get task, run task, remove task from list
	void Tasker::Run();

	vector<thread*> thread_vector;
	condition_variable run_cv;
	condition_variable end_cv;
	list<task_> task_list;
	atomic_bool exit;
	atomic_bool end_condition;
	mutex mx;

};

