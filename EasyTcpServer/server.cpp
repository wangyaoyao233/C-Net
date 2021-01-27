#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS // inet_ntoa()
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
		// recvbuf
		char recvBuf[1024] = {};
		int len = recv(_clientSock, recvBuf, sizeof(DataHeader), 0);
		// recvbuf  to header
		DataHeader* header = (DataHeader*)recvBuf;	
		
		if (len <= 0) {
			printf("client quit..\n");
			break;
		}

		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			// recvbuf to msg
			recv(_clientSock, recvBuf + sizeof(DataHeader), header->dataLen - sizeof(DataHeader), 0);
			Login* login = (Login*)recvBuf;

			printf("Login: userName = %s, passWord = %s\n", login->userName, login->password);
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

			printf("Logout: userName = %s\n", logout->userName);
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
		
	}

	// close
	closesocket(_sock);
	WSACleanup();
	return 0;
}