#include "EasyTcpClient.hpp"
#include <thread> // std::thread

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

	std::chrono::microseconds t(3000);
	std::this_thread::sleep_for(t);

	Login login[mCount];
	for (int i = 0; i < mCount; i++) {
		strcpy(login[i].userName, "client");
		strcpy(login[i].password, "4567");
	}
	const int len = sizeof(login);
	while (g_Run)
	{
		for (int i = begin; i < end; i++) {
			client[i]->SendData(login, len);
			//client[i]->OnRun();
		}
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

	
	tc.join();
	for (int i = 0; i < tCount; i++) {
		ts[i].join();
	}
	return 0;
}