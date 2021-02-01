#pragma once
#include "Semaphore.hpp"

#include <functional>

class CellThread
{
	typedef std::function<void(CellThread*)> EventCall;
public:
	// start thread
	void Start(EventCall onCreate,
		EventCall onRun,
		EventCall onDestory)
	{
		std::lock_guard<std::mutex> lock(_mutex);

		if (!_isRun) {
			_isRun = true;

			if (onCreate)
				_onCreate = onCreate;
			if (onRun)
				_onRun = onRun;
			if (onDestory)
				_onDestory = onDestory;

			std::thread t(&CellThread::OnWork, this);
			t.detach();
		}
	}

	// close thread
	void Close()
	{
		std::lock_guard<std::mutex> lock(_mutex);

		if (_isRun) {
			_isRun = false;
			_sem.Wait();
		}
	}

	// 在工作函数中退出, 不需要使用信号量
	// 如果使用会阻塞
	void Exit()
	{
		std::lock_guard<std::mutex> lock(_mutex);

		if (_isRun) {
			_isRun = false;
		}
	}

	// if the thread is on work
	bool IsRun() { return _isRun; }

protected:

	// 线程运行时的工作函数
	void OnWork()
	{
		if (_onCreate)
			_onCreate(this);
		if (_onRun)
			_onRun(this);
		if (_onDestory)
			_onDestory(this);

		_sem.WakeUp();
	}


private:
	EventCall _onCreate;
	EventCall _onRun;
	EventCall _onDestory;
	Semaphore _sem;
	std::mutex _mutex;
	bool _isRun = false;
};