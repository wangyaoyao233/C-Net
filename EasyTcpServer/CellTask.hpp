#pragma once
#include <thread> // std::thread
#include <mutex> // std::mutex
#include <list>
#include <functional>
#include "TimeStamp.hpp"


class CellTaskServer
{
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
				task();
			}
			_tasks.clear();
		}

	}

private:
	std::list<std::function<void()>> _tasks;
	std::list<std::function<void()>> _tasksBuf;
	std::mutex _mutex;
};