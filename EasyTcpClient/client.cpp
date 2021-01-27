#include "EasyTcpClient.hpp"
#include <thread> // std::thread

using namespace std;

void ThreadCmd(EasyTcpClient* client)
{
	while (true)
	{
		char cmdBuf[256];
		cin >> cmdBuf;

		if (0 == strcmp(cmdBuf, "exit")) {
			client->Close();
			break;
		}
		else if (0 == strcmp(cmdBuf, "Login")) {
			Login login;
			strcpy(login.userName, "client");
			strcpy(login.password, "4567");
			client->SendData(&login);
		}
	}

}

int main()
{

	EasyTcpClient* client = new EasyTcpClient;
	client->Init();
	client->Connect("127.0.0.1", 4567);
	

	// thread
	thread t1(ThreadCmd, client);

	while (client->IsRun())
	{
		client->OnRun();		
	}
	
	client->Close();
	delete client;
	t1.join();
	return 0;
}