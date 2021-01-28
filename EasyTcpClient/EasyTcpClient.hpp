#pragma once

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
#include "Message.hpp"

#define RECV_BUFF_SIZE (10240)

class EasyTcpClient
{
public:
	EasyTcpClient()
	{
		_sock = INVALID_SOCKET;
	}
	virtual ~EasyTcpClient()
	{
		Close();
	}

	// init socket
	void Init()
	{
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif // _WIN32

		if (INVALID_SOCKET != _sock) {
			printf("close pre socket = %d..\n", (int)_sock);
			Close();
		}
		// socket
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			printf("socket error..\n");
		}
	}

	// close socket
	void Close()
	{
		if (INVALID_SOCKET != _sock) {
		// close
#ifdef _WIN32
		closesocket(_sock);
		WSACleanup();
#else
		close(_sock);
#endif // _WIN32
		_sock = INVALID_SOCKET;
		}
	}

	// connect
	void Connect(const char* ip,unsigned short port)
	{
		if (INVALID_SOCKET == _sock) {
			Init();
		}
		// connect
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);//host to net ushort
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif // _WIN32
		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret) {
			printf("connect error..\n");
		}
	}

	bool OnRun()
	{
		if (IsRun()) {
			fd_set fdRead{};
			FD_ZERO(&fdRead);
			FD_SET(_sock, &fdRead);

			timeval t{ 0,0 };
			int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);
			if (ret < 0) {
				printf("<socket= %d>select quit..1\n", (int)_sock);
				Close();
				return false;
			}

			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);

				if (-1 == RecvData(_sock)) {
					printf("<socket= %d>select quit..2\n", (int)_sock);
					Close();
					return false;
				}
			}
			return true;
		}
		return false;
	}

	bool IsRun()
	{
		return INVALID_SOCKET != _sock;
	}

	char _recvBuf[RECV_BUFF_SIZE] = {}; // recv buffer
	char _msgBuf[RECV_BUFF_SIZE * 10] = {}; // the 2nd buffer
	int _lastPos = 0;
	// recv data
	int RecvData(SOCKET _cSock)
	{
		// recv buffer
		int len = recv(_cSock, _recvBuf, RECV_BUFF_SIZE, 0);
		if (len <= 0) {
			printf("client<%d> quit..\n", (int)_cSock);
			return -1;
		}
		// copy to 2nd buffer
		memcpy(_msgBuf + _lastPos, _recvBuf, len);
		_lastPos += len;

		while(_lastPos >= sizeof(DataHeader)){
			// recvbuf  to header
			DataHeader* header = (DataHeader*)_msgBuf;
			if (_lastPos >= header->dataLen) {
				int size = _lastPos - header->dataLen;
				OnNetMsg(header);
				memcpy(_msgBuf, _msgBuf + header->dataLen, size);
				_lastPos = size;
			}
			else {
				break;
			}
		}
	
		return 0;
	}

	void OnNetMsg(DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			LoginResult* ret = (LoginResult*)header;
			//printf("Login result = %d\n", ret->result);
		}
		break;
		case CMD_LOGOUT_RESULT:
		{
			LogoutResult* ret = (LogoutResult*)header;
			//printf("Logout result = %d\n", ret->result);
		}
		break;
		case CMD_NEWUSER_JOIN:
		{
			NewUserJoin* ret = (NewUserJoin*)header;
			//printf("New user join, socket = <%d>\n", ret->sock);
		}
		break;
		default:
		{
			printf("Unknow msg..\n");
		}
		break;
		}
	}


	// send data
	int SendData(DataHeader* header)
	{
		if (IsRun() && header) {
			return send(_sock, (const char*)header, header->dataLen, 0);
		}
		return SOCKET_ERROR;
	}
private:
	SOCKET _sock;
};