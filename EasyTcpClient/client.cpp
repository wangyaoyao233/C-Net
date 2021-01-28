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

int main()
{
	const int cCount = 1000;// FD_SETSIZE - 1;

	EasyTcpClient* client[cCount];

	for (int i = 0; i < cCount; i++) {
		client[i] = new EasyTcpClient;
	}

	for (int i = 0; i < cCount; i++) {
		client[i]->Init();
		client[i]->Connect("127.0.0.1", 4567);
	}

	// thread
	thread t1(ThreadCmd);

	Login login;
	strcpy(login.userName, "client");
	strcpy(login.password, "4567");

	while (g_Run)
	{
		for (int i = 0; i < cCount; i++) {
			client[i]->SendData(&login);
			//client[i]->OnRun();
		}			
	}
	for (int i = 0; i < cCount; i++) {
		client[i]->Close(); 
		delete client[i];
	}
	
	t1.join();
	return 0;
}