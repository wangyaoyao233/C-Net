#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS // inet_ntoa()
#define _CRT_SECURE_NO_WARNINGS // strcpy()
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_ERROR
};
struct DataHeader
{
	short dataLen;
	short cmd;
};

struct Login :public DataHeader
{
	Login() {
		this->dataLen = sizeof(Login);
		this->cmd = CMD_LOGIN;
	}
	char userName[32];
	char password[32];
};
struct LoginResult :public DataHeader
{
	LoginResult() {
		this->dataLen = sizeof(LoginResult);
		this->cmd = CMD_LOGIN_RESULT;
	}
	int result;
};

struct Logout :public DataHeader
{
	Logout() {
		this->dataLen = sizeof(Logout);
		this->cmd = CMD_LOGOUT;
	}
	char userName[32];
};
struct LogoutResult :public DataHeader
{
	LogoutResult() {
		this->dataLen = sizeof(LogoutResult);
		this->cmd = CMD_LOGOUT_RESULT;
	}
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
			// send msg
			Login login;
			strcpy(login.userName, "client");
			strcpy(login.password, "4567");
			send(_sock, (const char*)&login, sizeof(Login), 0);

			// recv msg
			LoginResult ret = {};
			recv(_sock, (char*)&ret, sizeof(LoginResult), 0);
			printf("Login result = %d\n", ret.result);
		}
		else if (0 == strcmp(cmdBuf, "Logout")) {
			// send msg body
			Logout logout;
			strcpy(logout.userName, "client");
			send(_sock, (const char*)&logout, sizeof(Logout), 0);

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