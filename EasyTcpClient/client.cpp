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

	// connect
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);//host to net ushort
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret) {
		printf("connect error..\n");
	}

	while (true)
	{
		char cmdBuf[256] = {};
		std::cin >> cmdBuf;
		if (0 == strcmp(cmdBuf, "exit")) {
			break;
		}
		else if (0 == strcmp(cmdBuf, "Login")) {
			// send msg header
			DataHeader header = { sizeof(Login),CMD_LOGIN };
			send(_sock, (const char*)&header, sizeof(DataHeader), 0);
			// send msg body
			Login login = { "client", "4567" };
			send(_sock, (const char*)&login, sizeof(Login), 0);
			// recv msg header
			DataHeader retHeader = {};
			recv(_sock, (char*)&retHeader, sizeof(DataHeader), 0);
			// recv msg body
			LoginResult ret = {};
			recv(_sock, (char*)&ret, sizeof(LoginResult), 0);
			printf("Login result = %d\n", ret.result);
		}
		else if (0 == strcmp(cmdBuf, "Logout")) {
			// send msg header
			DataHeader header = { sizeof(Logout),CMD_LOGOUT };
			send(_sock, (const char*)&header, sizeof(DataHeader), 0);
			// send msg body
			Logout logout = { "client" };
			send(_sock, (const char*)&logout, sizeof(Logout), 0);
			// recv msg header
			DataHeader retHeader = {};
			recv(_sock, (char*)&retHeader, sizeof(DataHeader), 0);
			// recv msg body
			LogoutResult ret = {};
			recv(_sock, (char*)&ret, sizeof(LogoutResult), 0);
			printf("Logout result = %d\n", ret.result);
		}
		else {
			// send
			printf("unknown cmd..\n");
		}

			
	}
	
	// close
	closesocket(_sock);
	WSACleanup();
	return 0;
}