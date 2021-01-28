#pragma once

#ifdef _WIN32

#define FD_SETSIZE 1024
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

#define RECV_BUFF_SIZE 10240

class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_msgBuf, 0, sizeof(_msgBuf));
		_lastPos = 0;
	}
	~ClientSocket()
	{

	}

	SOCKET Sockfd() { return _sockfd; }
	char* MsgBuf() { return _msgBuf; }
	int GetLastPos() { return _lastPos; }
	void SetLastPos(int pos) { _lastPos = pos; }

private:
	SOCKET _sockfd; // fd_set file desc set
	char _msgBuf[RECV_BUFF_SIZE * 10] = {}; // the 2nd buffer
	int _lastPos = 0;
};

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
			for (int i = 0; i < _clients.size(); i++) {
				closesocket(_clients[i]->Sockfd());
				delete _clients[i];
			}
			closesocket(_sock);
			WSACleanup();
#else
			for (int i = 0; i < g_Clients.size(); i++) {
				close(_clients[i]->Sockfd());
				delete _clients[i];
			}
			close(_sock);
#endif // _WIN32
			_sock = INVALID_SOCKET;
			_clients.clear();
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
		SOCKET cSock = INVALID_SOCKET;

		cSock = accept(_sock, (sockaddr*)&clientAddr, &addrLen);
		if (INVALID_SOCKET == cSock) {
			printf("accept error..\n");
		}
		else {
			printf("new client connect: socket = %d, IP = %s, nums = %d\n", (int)cSock, inet_ntoa(clientAddr.sin_addr), _clients.size());

			_clients.push_back(new ClientSocket(cSock));
		}
		return cSock;
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
			for (int i = 0; i < _clients.size(); i++) {
				FD_SET(_clients[i]->Sockfd(), &fdRead);
				if (_clients[i]->Sockfd() > maxSock) {
					maxSock = _clients[i]->Sockfd();
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

			for (int i = 0; i < _clients.size(); i++) {
				if (FD_ISSET(_clients[i]->Sockfd(), &fdRead)) {
					if (-1 == this->RecvData(_clients[i])) {
						auto iter = _clients.begin() + i;
						if (iter != _clients.end()) {
							delete _clients[i];
							_clients.erase(iter);
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

	char _recvBuf[RECV_BUFF_SIZE] = {};
	int RecvData(ClientSocket* client)
	{
		// recvbuf	
		int len = recv(client->Sockfd(), _recvBuf, RECV_BUFF_SIZE, 0);
		if (len <= 0) {
			printf("client<%d> quit..\n", (int)client->Sockfd());
			return -1;
		}
		// copy to 2nd buffer
		memcpy(client->MsgBuf() + client->GetLastPos(), _recvBuf, len);
		client->SetLastPos(client->GetLastPos() + len);

		while (client->GetLastPos() >= sizeof(DataHeader)) {
			// recvbuf  to header
			DataHeader* header = (DataHeader*)client->MsgBuf();
			if (client->GetLastPos() >= header->dataLen) {
				int size = client->GetLastPos() - header->dataLen;
				this->OnNetMsg(client->Sockfd(), header);
				memcpy(client->MsgBuf(), client->MsgBuf() + header->dataLen, size);
				client->SetLastPos(size);
			}
			else {
				break;
			}
		}

		return 0;
	}

	int SendData(SOCKET cSock, DataHeader* header)
	{
		if (IsRun() && header) {
			return send(cSock, (const char*)header, header->dataLen, 0);
		}
		return SOCKET_ERROR;
	}

	void SendDataToAll(DataHeader* header)
	{
		if (IsRun() && header) {
			for (int i = 0; i < _clients.size(); i++) {	
				SendData(_clients[i]->Sockfd(), header);
			}
		}
	}

	virtual void OnNetMsg(SOCKET cSock, DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login* login = (Login*)header;
			//printf("client<%d> Login: userName = %s, passWord = %s\n", (int)cSock, login->userName, login->password);
			// send msg
			//LoginResult ret;
			//ret.result = 1;
			//this->SendData(cSock, &ret);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			//printf("client<%d> Logout: userName = %s\n", (int)cSock, logout->userName);
			// send msg
			//LogoutResult ret;
			//ret.result = 1;
			//this->SendData(cSock, &ret);
		}
		break;
		default:
		{
			
		}
		break;
		}
	}

private:
	SOCKET _sock;
	std::vector<ClientSocket*> _clients;
};