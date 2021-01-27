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
#include <vector>
#include "Message.hpp"

class EasyTcpServer
{
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
	}
	~EasyTcpServer()
	{
		if (INVALID_SOCKET != _sock) {
			Close();
		}
	}

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

	void Close()
	{
		if (INVALID_SOCKET != _sock) {
			// close
#ifdef _WIN32
			for (int i = 0; i < _Clients.size(); i++) {
				closesocket(_Clients[i]);
			}
			closesocket(_sock);
			WSACleanup();
#else
			for (int i = 0; i < g_Clients.size(); i++) {
				close(g_Clients[i]);
			}
			close(_sock);
#endif // _WIN32
			_sock = INVALID_SOCKET;
		}
	}

	int Bind(const char* ip, unsigned short port)
	{
		if (INVALID_SOCKET == _sock) {
			Init();
		}
		// bind
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(4567);//host to net ushort
#ifdef _WIN32
		if (ip) {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("127.0.0.1");
		}
#else
		if (ip) {
			_sin.sin_addr.s_addr = inet_addr("127.0.0.1");
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;//inet_addr("127.0.0.1");
		}	
#endif // _WIN32
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret) {
			printf("bind port = <%d> error...\n", port);
		}
		else {
			printf("bind port = <%d> success...\n", port);
		}
		return ret;
	}

	int Listen(int n)
	{
		// listen
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret) {
			printf("<sock> = %d, listen error...\n",(int)_sock);
		}
		else {
			printf("<sock> = %d, listen success...\n", (int)_sock);
		}
		return ret;
	}

	SOCKET Accept()
	{
		// accept
		sockaddr_in clientAddr = {};
		int addrLen = sizeof(sockaddr_in);
		SOCKET clientSock = INVALID_SOCKET;

		clientSock = accept(_sock, (sockaddr*)&clientAddr, &addrLen);
		if (INVALID_SOCKET == clientSock) {
			printf("accept error..\n");
		}
		else {
			printf("new client connect: socket = %d, IP = %s\n", (int)clientSock, inet_ntoa(clientAddr.sin_addr));

			NewUserJoin userJoin{};
			userJoin.sock = clientSock;
			for (int i = 0; i < _Clients.size(); i++) {
				send(_Clients[i], (const char*)&userJoin, sizeof(NewUserJoin), 0);
			}

			_Clients.push_back(clientSock);
		}
		return clientSock;
	}


	bool OnRun()
	{
		if (IsRun()) {
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
			for (int i = 0; i < _Clients.size(); i++) {
				FD_SET(_Clients[i], &fdRead);
				if (_Clients[i] > maxSock) {
					maxSock = _Clients[i];
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
				Close();
				return false;
			}

			// accept
			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);

				this->Accept();
			}

			for (int i = 0; i < _Clients.size(); i++) {
				if (FD_ISSET(_Clients[i], &fdRead)) {
					if (-1 == this->RecvData(_Clients[i])) {
						auto iter = _Clients.begin() + i;
						if (iter != _Clients.end()) {
							_Clients.erase(iter);
						}
					}
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

	int RecvData(SOCKET clientSock)
	{
		// recvbuf
		char recvBuf[1024] = {};
		int len = recv(clientSock, recvBuf, sizeof(DataHeader), 0);
		// recvbuf  to header
		DataHeader* header = (DataHeader*)recvBuf;
		if (len <= 0) {
			printf("client<%d> quit..\n", (int)clientSock);
			return -1;
		}

		// recvbuf to msg
		recv(clientSock, recvBuf + sizeof(DataHeader), header->dataLen - sizeof(DataHeader), 0);

		this->OnNetMsg(clientSock, header);

		return 0;
	}

	int SendData(SOCKET clientSock, DataHeader* header)
	{
		if (IsRun() && header) {
			return send(clientSock, (const char*)&header, header->dataLen, 0);
		}
		return SOCKET_ERROR;
	}

	void SendDataToAll(DataHeader* header)
	{
		if (IsRun() && header) {
			for (int i = 0; i < _Clients.size(); i++) {	
				SendData(_Clients[i], header);
			}
		}
	}

	virtual void OnNetMsg(SOCKET clientSock, DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			printf("client<%d> Login: userName = %s, passWord = %s\n", (int)clientSock, login->userName, login->password);
			// send msg
			LoginResult ret;
			ret.result = 1;
			send(clientSock, (const char*)&ret, sizeof(LoginResult), 0);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			printf("client<%d> Logout: userName = %s\n", (int)clientSock, logout->userName);
			// send msg
			LogoutResult ret;
			ret.result = 1;
			send(clientSock, (const char*)&ret, sizeof(LogoutResult), 0);
		}
		break;
		default:
		{
			DataHeader header = { 0, CMD_ERROR };
			send(clientSock, (const char*)&header, sizeof(DataHeader), 0);
		}
		break;
		}
	}

private:
	SOCKET _sock;
	std::vector<SOCKET> _Clients;
};