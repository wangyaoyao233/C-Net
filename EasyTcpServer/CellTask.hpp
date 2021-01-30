#pragma once
#include <thread> // std::thread
#include <mutex> // std::mutex
#include <list>
#include "TimeStamp.hpp"

// base task class
class ITask
{
public:
	ITask() {}
	virtual ~ITask() {}

	virtual void DoTask() = 0;
};

class CellTaskServer
{
public:
	CellTaskServer() {}
	~CellTaskServer() {}

	void AddTask(ITask* task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(task);
	}

	void Start()
	{
		std::thread t(&CellTaskServer::OnRun, this);
		t.detach();
	}

private:
	void OnRun()
	{
		while (true)
		{
			if (!_tasksBuf.empty()) {
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto task : _tasksBuf) {
					_tasks.push_back(task);
				}
				_tasksBuf.clear();
			}
			if (_tasks.empty()) {
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			for (auto task : _tasks) {
				task->DoTask();
				delete task;
			}
			_tasks.clear();
		}

	}

private:
	std::list<ITask*> _tasks;
	std::list<ITask*> _tasksBuf;
	std::mutex _mutex;
};