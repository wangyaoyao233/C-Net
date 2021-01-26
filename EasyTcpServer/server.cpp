#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS // inet_ntoa()
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

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

	char recvBuf[256] = {};
	
	while (true)
	{
		int len = recv(_clientSock, recvBuf, 256, 0);
		if (len <= 0) {
			printf("client quit..\n");
			break;
		}
		printf("recv msg = %s\n", recvBuf);

		if (0 == strcmp(recvBuf, "GetName")) {
			char sendBuf[] = "Server";
			// send
			send(_clientSock, sendBuf, strlen(sendBuf) + 1, 0);
		}
		else if (0 == strcmp(recvBuf, "GetAge")) {
			char sendBuf[] = "4567";
			// send
			send(_clientSock, sendBuf, strlen(sendBuf) + 1, 0);
		}
		else {
			char sendBuf[] = "???";
			// send
			send(_clientSock, sendBuf, strlen(sendBuf) + 1, 0);
		}
	}


	// close
	closesocket(_sock);


	WSACleanup();
	return 0;
}