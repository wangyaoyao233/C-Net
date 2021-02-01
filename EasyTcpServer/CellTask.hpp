#pragma once

#include "TimeStamp.hpp"
#include "CellThread.h"

#include <thread> // std::thread
#include <mutex> // std::mutex
#include <list>
#include <functional>

class CellTaskServer
{
public:
	int _serverid = -1;

public:
	CellTaskServer() {}
	~CellTaskServer() {}

	void AddTask(std::function<void()> task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(task);
	}

	void Start()
	{
		_thread.Start(nullptr, 
			[&](CellThread* t) {
				OnRun(t);
			},
			nullptr);
	}

	void Close()
	{
		_thread.Close();
	}

private:
	void OnRun(CellThread* pThread)
	{
		while (pThread->IsRun())
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
				task();
			}
			_tasks.clear();
		}
		printf("CellTaskServer %d OnRun exit\n", _serverid);
	}

private:
	std::list<std::function<void()>> _tasks;
	std::list<std::function<void()>> _tasksBuf;
	CellThread _thread;
	std::mutex _mutex;
};