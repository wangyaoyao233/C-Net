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
	CMD_ERROR,
	CMD_NEWUSER_JOIN
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

struct NewUserJoin :public DataHeader
{
	NewUserJoin() {
		this->dataLen = sizeof(NewUserJoin);
		this->cmd = CMD_NEWUSER_JOIN;
	}
	int sock;
};

int Processor(SOCKET _clientSock)
{
	// recvbuf
	char recvBuf[1024] = {};
	int len = recv(_clientSock, recvBuf, sizeof(DataHeader), 0);
	// recvbuf  to header
	DataHeader* header = (DataHeader*)recvBuf;

	if (len <= 0) {
		printf("client<%d> quit..\n", (int)_clientSock);
		return -1;
	}

	switch (header->cmd)
	{
	case CMD_LOGIN_RESULT:
	{
		// recvbuf to msg
		recv(_clientSock, recvBuf + sizeof(DataHeader), header->dataLen - sizeof(DataHeader), 0);
		LoginResult* ret = (LoginResult*)recvBuf;

		printf("Login result = %d\n", ret->result);
	}
	break;
	case CMD_LOGOUT_RESULT:
	{
		// recvbuf to msg
		recv(_clientSock, recvBuf + sizeof(DataHeader), header->dataLen - sizeof(DataHeader), 0);
		LogoutResult* ret = (LogoutResult*)recvBuf;

		printf("Logout result = %d\n", ret->result);
	}
	break;
	case CMD_NEWUSER_JOIN:
	{
		// recvbuf to msg
		recv(_clientSock, recvBuf + sizeof(DataHeader), header->dataLen - sizeof(DataHeader), 0);
		NewUserJoin* ret = (NewUserJoin*)recvBuf;

		printf("New user join, socket = <%d>\n", ret->sock);
	}
		break;
	default:
	{
	}
	break;
	}
	return 0;
}

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
		fd_set fdRead{};
		FD_ZERO(&fdRead);
		FD_SET(_sock, &fdRead);

		timeval t{ 0,0 };
		int ret = select(_sock, &fdRead, nullptr, nullptr, &t);
		if (ret < 0) {
			printf("select quit..1\n");
			break;
		}

		if (FD_ISSET(_sock, &fdRead)) {
			FD_CLR(_sock, &fdRead);

			if (-1 == Processor(_sock)) {
				printf("select quit..2\n");
				break;
			}
		}

		Login login;
		strcpy(login.userName, "client");
		strcpy(login.password, "4567");
		send(_sock, (const char*)&login, sizeof(Login), 0);
		Sleep(1000);
			
	}
	
	// close
	closesocket(_sock);
	WSACleanup();
	return 0;
}