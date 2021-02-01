#pragma once

#include "TimeStamp.hpp"
#include "Semaphore.hpp"

#include <thread> // std::thread
#include <mutex> // std::mutex
#include <list>
#include <functional>

class CellTaskServer
{
public:
	int _serverid = -1;

public:
	CellTaskServer() 
	{
		_isRun = false;
	}
	~CellTaskServer() {}

	void AddTask(std::function<void()> task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(task);
	}

	void Start()
	{
		_isRun = true;
		std::thread t(&CellTaskServer::OnRun, this);
		t.detach();
	}

	void Close()
	{
		if (_isRun) {
			printf("CellTaskServer %d close begin\n", _serverid);
			_isRun = false;
			
			_sem.Wait(); // wait until Onrun exit

			printf("CellTaskServer %d close end\n", _serverid);
		}

	}

private:
	void OnRun()
	{
		while (_isRun)
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
		_sem.WakeUp();
	}

private:
	std::list<std::function<void()>> _tasks;
	std::list<std::function<void()>> _tasksBuf;
	std::mutex _mutex;
	bool _isRun = false;
	Semaphore _sem;
};