#include "Alloc.h"
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

class MyServer :public EasyTcpServer
{
public:
	void OnNetLeave(ClientSocket* client) override
	{
		EasyTcpServer::OnNetLeave(client);
	}

	void OnNetMsg(CellServer* cellServer, ClientSocket* client, DataHeader* header) override
	{
		EasyTcpServer::OnNetMsg(cellServer, client, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			//Login* login = (Login*)header;
			//printf("client<%d> Login: userName = %s, passWord = %s\n", (int)cSock, login->userName, login->password);

			// send msg
			LoginResult* ret = new LoginResult();
			ret->result = 1;

			cellServer->AddSendTask(client, ret);
		}
		break;
		case CMD_LOGOUT:
		{
			//Logout* logout = (Logout*)header;
			//printf("client<%d> Logout: userName = %s\n", (int)cSock, logout->userName);
			// send msg
			//LogoutResult ret;
			//ret.result = 1;
			//this->SendData(cSock, &ret);
		}
		break;
		default:
		{

		}
		break;
		}
	}

	void OnNetJoin(ClientSocket* client) override
	{
		EasyTcpServer::OnNetJoin(client);
	}

	void OnNetRecv(ClientSocket* client) override
	{
		EasyTcpServer::OnNetRecv(client);
	}
};

int main()
{
	EasyTcpServer* server = new MyServer();
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