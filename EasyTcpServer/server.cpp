#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS // inet_ntoa()
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

enum CMD
{
	CMD_LOGIN,
	CMD_LOGOUT,
	CMD_ERROR
};
struct DataHeader
{
	short dataLen;
	short cmd;
};

struct Login
{
	char userName[32];
	char password[32];
};
struct LoginResult
{
	int result;
};

struct Logout
{
	char userName[32];
};
struct LogoutResult
{
	int result;
};

int main()
{
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	// socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == _sock) {
		printf("socket error..\n");
	}

	// bind
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);//host to net ushort
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("127.0.0.1");
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in))) {
		printf("bind error...\n");
	}

	// listen
	if (SOCKET_ERROR == listen(_sock, 5)) {
		printf("listen error...\n");
	}

	// accept
	sockaddr_in _clientAddr = {};
	int addrLen = sizeof(sockaddr_in);
	SOCKET _clientSock = INVALID_SOCKET;

	_clientSock = accept(_sock, (sockaddr*)&_clientAddr, &addrLen);
	if (INVALID_SOCKET == _clientSock) {
		printf("accept error..\n");
	}
	else {
		printf("new client connect: socket = %d, IP = %s\n", (int)_clientSock, inet_ntoa(_clientAddr.sin_addr));
	}

	
	while (true)
	{
		// recv msg header
		DataHeader header = {};
		int len = recv(_clientSock, (char*)&header, sizeof(DataHeader), 0);
		if (len <= 0) {
			printf("client quit..\n");
			break;
		}
		printf("recv cmd = %d\n", header.cmd);

		switch (header.cmd)
		{
		case CMD_LOGIN:
		{
			// recv msg body
			Login login = {};
			recv(_clientSock, (char*)&login, sizeof(Login), 0);

			printf("Login: userName = %s, passWord = %s\n", login.userName, login.password);
			// send msg header
			send(_clientSock, (const char*)&header, sizeof(DataHeader), 0);
			// send msg body
			LoginResult ret = { 1 };
			send(_clientSock, (const char*)&ret, sizeof(LoginResult), 0);
		}
			break;
		case CMD_LOGOUT:
		{
			// recv msg body
			Logout logout = {};
			recv(_clientSock, (char*)&logout, sizeof(Logout), 0);

			printf("Logout: userName = %s\n", logout.userName);
			// send msg header
			send(_clientSock, (const char*)&header, sizeof(DataHeader), 0);
			// send msg body
			LogoutResult ret = { 1 };
			send(_clientSock, (const char*)&ret, sizeof(LogoutResult), 0);
		}
			break;
		default:
		{
			header.cmd = CMD_ERROR;
			header.dataLen = 0;
			send(_clientSock, (const char*)&header, sizeof(DataHeader), 0);
		}
			break;
		}
		
	}

	// close
	closesocket(_sock);
	WSACleanup();
	return 0;
}