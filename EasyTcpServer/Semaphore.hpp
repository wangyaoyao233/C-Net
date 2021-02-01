#pragma once
#include <chrono>
#include <thread>
#include <condition_variable>

class Semaphore
{
public:

	// 阻塞当前线程
	void Wait()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		if (--_wait < 0) {
			_cv.wait(lock, [&]() {
				return _wakeup > 0;
				});
			--_wakeup;
		}
	}

	// 唤醒当前线程
	void WakeUp()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (++_wait <= 0) {
			++_wakeup;
			_cv.notify_one();
		}			
	}
private:
	std::condition_variable _cv;
	std::mutex _mutex;
	int _wait = 0;
	int _wakeup = 0;
};