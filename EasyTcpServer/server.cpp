#include "EasyTcpServer.hpp"

using namespace std;

int main()
{
	EasyTcpServer* server = new EasyTcpServer;
	server->Init();
	server->Bind(nullptr, 4567);
	server->Listen(5);

	while (server->IsRun())
	{
		server->OnRun();		
	}

	server->Close();
	delete server;
	return 0;
}