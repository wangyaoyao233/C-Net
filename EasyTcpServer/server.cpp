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
	void OnNetLeave(std::shared_ptr<CellClient> client) override
	{
		EasyTcpServer::OnNetLeave(client);
	}

	void OnNetMsg(CellServer* cellServer, std::shared_ptr<CellClient> client, Netmsg_DataHeader* header) override
	{
		EasyTcpServer::OnNetMsg(cellServer, client, header);
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			//Login* login = (Login*)header;
			//printf("client<%d> Login: userName = %s, passWord = %s\n", (int)cSock, login->userName, login->password);

			// send msg
			auto ret = std::make_shared<Netmsg_LoginR>();
			ret->result = 1;

			cellServer->AddSendTask(client, std::shared_ptr<Netmsg_DataHeader>(ret));
		}
		break;
		case CMD_LOGOUT:
		{
		}
		break;
		case CMD_HEART_C2S:
		{
			// reset heart
			client->RestLife();

			auto ret = std::make_shared<Netmsg_Heart_S2C>();
			cellServer->AddSendTask(client, std::shared_ptr<Netmsg_DataHeader>(ret));
		}
		break;
		default:
		{
		}
		break;
		}
	}

	void OnNetJoin(std::shared_ptr<CellClient> client) override
	{
		EasyTcpServer::OnNetJoin(client);
	}

	void OnNetRecv(std::shared_ptr<CellClient> client) override
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