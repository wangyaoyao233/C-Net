#pragma once
#include <chrono>

using namespace std::chrono;
class TimeStamp
{
public:
	TimeStamp() { Update(); }
	~TimeStamp() {}

	void Update()
	{
		_begin = high_resolution_clock::now();
	}

	double GetElapsedTimeInSec()
	{
		return this->GetElapsedTimeInMicroSec() * 0.000001;
	}

	double GetElapsedTimeInMilliSec()
	{
		return this->GetElapsedTimeInMicroSec() * 0.001;
	}

	long long GetElapsedTimeInMicroSec()
	{
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}

private:
	time_point<high_resolution_clock> _begin;
};