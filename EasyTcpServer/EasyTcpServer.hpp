#pragma once

#ifdef _WIN32

#define FD_SETSIZE 2510
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
#include <thread> // std::thread
#include <mutex> // std::mutex
#include <atomic> // std::atomic
#include "Message.hpp"
#include "TimeStamp.hpp"

#define RECV_BUFF_SIZE 10240
#define CELLSERVER_THREAD_NUM 4

class ClientSocket;
class CellServer;
class EasyTcpServer;

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

class INetEvent
{
public:
	virtual void OnLeave(ClientSocket* client) = 0;
};


class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_thread = nullptr;
		_recvCnt = 0;
		_netEvent = nullptr;
	}
	~CellServer()
	{
		Close();
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
#else
			for (int i = 0; i < g_Clients.size(); i++) {
				close(_clients[i]->Sockfd());
				delete _clients[i];
			}
#endif // _WIN32
			_clients.clear();
		}
	}



	bool OnRun()
	{
		while (IsRun()) {
			// get client from clients buffer queue
			if (!_clientsBuffer.empty()) {
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto client : _clientsBuffer) {
					_clients.push_back(client);
				}
				_clientsBuffer.clear();
			}
			// if no client
			if (_clients.empty()) {
				std::chrono::microseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			// select
			fd_set fdRead{};
			FD_ZERO(&fdRead);

			int maxSock = _clients[0]->Sockfd();;
			for (int i = 0; i < _clients.size(); i++) {
				FD_SET(_clients[i]->Sockfd(), &fdRead);
				if (_clients[i]->Sockfd() > maxSock) {
					maxSock = _clients[i]->Sockfd();
				}
			}

			/// <summary>
			/// nfds 是一个整数值, 是指fd_set集合中所有描述符(socket)的范围, 而不是数量, 即是所有文件描述符的最大值+1
			/// </summary>
			/// <returns></returns>
			int ret = select(maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			if (ret < 0) {
				printf("select quit..\n");
				Close();
				return false;
			}

			for (int i = 0; i < _clients.size(); i++) {
				if (FD_ISSET(_clients[i]->Sockfd(), &fdRead)) {
					if (-1 == this->RecvData(_clients[i])) {
						auto iter = _clients.begin() + i;
						if (iter != _clients.end()) {
							// leave event
							if(_netEvent)
								_netEvent->OnLeave(_clients[i]);

							delete _clients[i];
							_clients.erase(iter);
						}
					}
				}
			}

		}

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

	virtual void OnNetMsg(SOCKET cSock, DataHeader* header)
	{
		_recvCnt++;
		//auto t1 = _time.GetElapsedTimeInSec();
		//if (t1 >= 1.0) {
		//	printf("time<%lf>,client nums<%d> recvCnt<%d>\n", t1, _clients.size(), _recvCnt);
		//	_time.Update();
		//	_recvCnt = 0;
		//}

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

	void AddClient(ClientSocket* client)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_clientsBuffer.push_back(client);
	}

	void Start()
	{
		_thread = new std::thread(&CellServer::OnRun, this);
	}

	size_t GetClientCount()
	{
		return _clients.size() + _clientsBuffer.size();
	}

	void SetEventObj(INetEvent* e)
	{
		_netEvent = e;
	}

private:
	SOCKET _sock;
	std::vector<ClientSocket*> _clients;
	// clients buffer queue
	std::vector<ClientSocket*> _clientsBuffer; 
	std::mutex _mutex;
	std::thread* _thread;
	INetEvent* _netEvent;
public:
	std::atomic<int> _recvCnt;
};


class EasyTcpServer: public INetEvent
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
			//printf("new client connect: socket = %d, IP = %s, nums = %d\n", (int)cSock, inet_ntoa(clientAddr.sin_addr), _clients.size());

			this->AddClientToCellServer(new ClientSocket(cSock));
			
		}
		return cSock;
	}

	void AddClientToCellServer(ClientSocket* client)
	{
		_clients.push_back(client);
		// look for the min num in cellServers
		auto min = _cellServers[0];
		for (auto cellServer : _cellServers){
			if (cellServer->GetClientCount() < min->GetClientCount()) {
				min = cellServer;
			}
		}
		min->AddClient(client);
	}

	void Start()
	{
		for (int i = 0; i < CELLSERVER_THREAD_NUM; i++) {
			auto ser = new CellServer(_sock);
			_cellServers.push_back(ser);
			ser->SetEventObj(this);
			ser->Start();
		}
	}


	bool OnRun()
	{
		if (IsRun()) {
			this->TimeForMsg();
			// select
			fd_set fdRead{};
			FD_ZERO(&fdRead);
			FD_SET(_sock, &fdRead);

			timeval t{ 0,0 };
			/// <summary>
			/// nfds 是一个整数值, 是指fd_set集合中所有描述符(socket)的范围, 而不是数量, 即是所有文件描述符的最大值+1
			/// </summary>
			/// <returns></returns>
			int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);
			if (ret < 0) {
				printf("select quit..\n");
				Close();
				return false;
			}

			// accept
			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);

				this->Accept();
				// all clients connected, to recv data
				return true;
			}

			return true;
		}
		return false;
	}

	bool IsRun()
	{
		return INVALID_SOCKET != _sock;
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

	virtual void TimeForMsg()
	{
		auto t1 = _time.GetElapsedTimeInSec();
		if( t1 >= 1.0){
			int recvCnt = 0;
			for (auto ser : _cellServers) {
				recvCnt += ser->_recvCnt;
				ser->_recvCnt = 0;
			}

			printf("thread<%d>, time<%lf>,client nums<%d> recvCnt<%d>\n",_cellServers.size(), t1, _clients.size(), (int)(recvCnt / t1));
			_time.Update();
		}
	}

	virtual void OnLeave(ClientSocket* client)
	{
		for (int i = 0; i < _clients.size(); i++) {
			if (_clients[i] == client) {
				auto iter = _clients.begin() + i;
				if (iter != _clients.end()) {
					_clients.erase(iter);
				}
			}
		}
	}

private:
	SOCKET _sock;
	std::vector<ClientSocket*> _clients;
	std::vector<CellServer*> _cellServers;
	TimeStamp _time;
};