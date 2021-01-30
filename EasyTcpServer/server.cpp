#include "EasyTcpServer.hpp"
#include <thread>

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

//class MyServer :public EasyTcpServer
//{
//public:
//	virtual void OnNetLeave(ClientSocket* client)
//	{
//		_clientCnt--;
//	}
//
//	virtual void OnNetMsg(ClientSocket* client, DataHeader* header)
//	{
//		_msgCnt++;
//	}
//
//	virtual void OnNetJoin(ClientSocket* client)
//	{
//		_clientCnt++;
//	}
//};

int main()
{
	EasyTcpServer* server = new EasyTcpServer;
	server->Init();
	server->Bind(nullptr, 4567);
	server->Listen(5);
	server->Start(4);

	std::thread t1(ThreadCmd);

	while (g_Run)
	{
		server->OnRun();		
	}

	server->Close();
	delete server;
	t1.join();
	return 0;
}