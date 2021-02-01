#include "EasyTcpClient.hpp"
#include <thread> // std::thread
#include <atomic> // std::atomic

using namespace std;
bool g_Run = true;
void ThreadCmd()
{
	while (true)
	{
		char cmdBuf[256];
		cin >> cmdBuf;

		if (0 == strcmp(cmdBuf, "exit")) {
			g_Run = false;
			break;
		}
	}
}




const int cCount = 1000;// FD_SETSIZE - 1;
const int tCount = 4;
const int mCount = 10;
EasyTcpClient* client[cCount];
std::atomic<int> sendCnt = 0;
std::atomic<int> readyCnt = 0;

void ThreadRecv(int begin, int end)
{
	while (g_Run)
	{
		for (int i = begin; i < end; i++) {
			client[i]->OnRun();
		}
	}
}



void ThreadSend(int id)
{
	// id: 1~5
	int begin = (id - 1) * (cCount / tCount);
	int end = id * (cCount / tCount);

	for (int i = begin; i < end; i++) {
		client[i] = new EasyTcpClient();
	}

	for (int i = begin; i < end; i++) {
		client[i]->Init();
		client[i]->Connect("127.0.0.1", 4567);
	}
	printf("thread id=%d, connect<begin=%d, end=%d>\n", id, begin, end);

	// all thread ready, begin to send
	readyCnt++;
	while (readyCnt < tCount)
	{
		std::chrono::microseconds t(10);
		std::this_thread::sleep_for(t);
	}

	// start recv thread
	std::thread tr(ThreadRecv, begin, end);
	tr.detach();

	Netmsg_Login login[mCount];
	for (int i = 0; i < mCount; i++) {
		strcpy(login[i].userName, "client");
		strcpy(login[i].password, "4567");
	}
	const int len = sizeof(login);
	while (g_Run)
	{
		for (int i = begin; i < end; i++) {
			if (SOCKET_ERROR != client[i]->SendData(login, len)) {
				sendCnt++;
			}
		}
		//std::chrono::microseconds t(10);
		//std::this_thread::sleep_for(t);
	}

	for (int i = begin; i < end; i++) {
		client[i]->Close();
		delete client[i];
	}
}

int main()
{

	// cmd thread
	thread tc(ThreadCmd);

	// send thread
	thread ts[tCount];
	for (int i = 0; i < tCount; i++) {
		ts[i] = thread(ThreadSend, i + 1);
	}

	TimeStamp time;
	while (g_Run){
		auto t1 = time.GetElapsedTimeInSec();
		if (t1 >= 1.0) {
			printf("thread<%d>, clients<%d>, time<%lf>, send<%d>\n", tCount, cCount, t1, (int)(sendCnt / t1));
			sendCnt = 0;
			time.Update();
		}
		Sleep(1);
	}
	
	tc.join();
	for (int i = 0; i < tCount; i++) {
		ts[i].join();
	}
	return 0;
}