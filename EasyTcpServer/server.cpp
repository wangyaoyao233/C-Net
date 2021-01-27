#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS // inet_ntoa()
#define _CRT_SECURE_NO_WARNINGS // strcpy()
#include <WinSock2.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")
#else
#define SOCKET int
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#include <unistd.h> // uni std
#include <arpa/inet.h> // winsock2.h
#endif // _WIN32

#include <iostream>
#include <string>
#include <vector>

using namespace std;

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

std::vector<SOCKET> g_Clients;

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
	case CMD_LOGIN:
	{
		// recvbuf to msg
		recv(_clientSock, recvBuf + sizeof(DataHeader), header->dataLen - sizeof(DataHeader), 0);
		Login* login = (Login*)recvBuf;

		printf("client<%d> Login: userName = %s, passWord = %s\n", (int)_clientSock, login->userName, login->password);
		// send msg
		LoginResult ret;
		ret.result = 1;
		send(_clientSock, (const char*)&ret, sizeof(LoginResult), 0);
	}
	break;
	case CMD_LOGOUT:
	{
		// recvbuf to msg
		recv(_clientSock, recvBuf + sizeof(DataHeader), header->dataLen - sizeof(DataHeader), 0);
		Logout* logout = (Logout*)recvBuf;

		printf("client<%d> Logout: userName = %s\n", (int)_clientSock, logout->userName);
		// send msg
		LogoutResult ret;
		ret.result = 1;
		send(_clientSock, (const char*)&ret, sizeof(LogoutResult), 0);
	}
	break;
	default:
	{
		DataHeader header = { 0, CMD_ERROR };
		send(_clientSock, (const char*)&header, sizeof(DataHeader), 0);
	}
	break;
	}
	return 0;
}

int main()
{
#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
#endif // _WIN32

	// socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == _sock) {
		printf("socket error..\n");
	}

	// bind
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);//host to net ushort
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("127.0.0.1");
#else
	_sin.sin_addr.s_addr = INADDR_ANY;//inet_addr("127.0.0.1");
#endif // _WIN32
	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in))) {
		printf("bind error...\n");
	}

	// listen
	if (SOCKET_ERROR == listen(_sock, 5)) {
		printf("listen error...\n");
	}

	while (true)
	{
		// select
		fd_set fdRead{};
		fd_set fdWrite{};
		fd_set fdExcp{};
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExcp);
		
		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExcp);

		int maxSock = _sock;
		for (int i = 0; i < g_Clients.size(); i++) {
			FD_SET(g_Clients[i], &fdRead);
			if (g_Clients[i] > maxSock) {
				maxSock = g_Clients[i];
			}
		}

		timeval t{ 0,0 };
		/// <summary>
		/// nfds 是一个整数值, 是指fd_set集合中所有描述符(socket)的范围, 而不是数量, 即是所有文件描述符的最大值+1
		/// </summary>
		/// <returns></returns>
		int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExcp, &t);
		if (ret < 0) {
			printf("select quit..\n");
			break;
		}

		if (FD_ISSET(_sock, &fdRead)) {
			FD_CLR(_sock, &fdRead);

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

				NewUserJoin userJoin{};
				userJoin.sock = _clientSock;
				for (int i = 0; i < g_Clients.size(); i++) {
					send(g_Clients[i], (const char*)&userJoin, sizeof(NewUserJoin), 0);
				}

				g_Clients.push_back(_clientSock);
			}


		}

		for (int i = 0; i < g_Clients.size(); i++) {
			if (FD_ISSET(g_Clients[i], &fdRead)) {
				if (-1 == Processor(g_Clients[i])) {
					auto iter = g_Clients.begin() + i;
					if (iter != g_Clients.end()) {
						g_Clients.erase(iter);
					}
				}
			}
		}
		
		
	}

	// close
#ifdef _WIN32
	for (int i = 0; i < g_Clients.size(); i++) {
		closesocket(g_Clients[i]);
	}
	closesocket(_sock);
	WSACleanup();
#else
	for (int i = 0; i < g_Clients.size(); i++) {
		close(g_Clients[i]);
	}
	close(_sock);
#endif // _WIN32

	return 0;
}