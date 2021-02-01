#pragma once
#include <chrono>
#include <thread>

class Semaphore
{
public:

	void Wait()
	{
		_isWaitExit = true;
		while (_isWaitExit)// 阻塞等待 OnRun退出
		{// 信号量
			std::chrono::milliseconds t(1);
			std::this_thread::sleep_for(t);
		}
	}

	void WakeUp()
	{
		if (_isWaitExit)
			_isWaitExit = false;
		else
			printf("Semaphore WakeUp error..\n");
	}
private:
	bool _isWaitExit = false;
};