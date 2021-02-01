#pragma once

#include "Common.h"
#include "CellTask.hpp"
#include "CellClient.hpp"
#include "CellServer.hpp"
#include "INetEvent.hpp"
#include "CellThread.h"


#include <vector>
#include <thread> // std::thread
#include <atomic> // std::atomic


class EasyTcpServer: public INetEvent
{
public:
	EasyTcpServer()
	{
		_sock = INVALID_SOCKET;
		_msgCnt = 0;
		_recvCnt = 0;
		_clientCnt = 0;
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
		printf("EasyTcpServer, close 1\n");

		_thread.Close();

		if (INVALID_SOCKET != _sock) {
			_cellServers.clear();

			// close
#ifdef _WIN32
			closesocket(_sock);
			WSACleanup();
#else
			close(_sock);
#endif // _WIN32
			_sock = INVALID_SOCKET;
		}

		printf("EasyTcpServer, close 2\n");
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
		int ret = ::bind(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
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

			// add new client to cellServers
			// 注意: std::make_shared 与 std::shared_ptr(new) 不同, 前者直接包装成一个, 后者先 new 对象,然后包装
			this->AddClientToCellServer(std::shared_ptr<CellClient>(new CellClient(cSock)));
			
		}
		return cSock;
	}

	void AddClientToCellServer(std::shared_ptr<CellClient> client)
	{
		// look for the min num in cellServers
		auto min = _cellServers[0];
		for (auto cellServer : _cellServers){
			if (cellServer->GetClientCount() < min->GetClientCount()) {
				min = cellServer;
			}
		}
		min->AddClient(client);
		
	}

	void Start(int cellServer)
	{
		for (int i = 0; i < cellServer; i++) {
			auto ser = std::make_shared<CellServer>(i + 1);
			_cellServers.push_back(ser);
			// regist net event
			ser->SetEventObj(this);
			// start client msg process thread
			ser->Start();
		}
		
		_thread.Start(nullptr,
			[this](CellThread* t) {
				OnRun(t);
			},nullptr);
	}


	void OnRun(CellThread* pThread)
	{
		while (pThread->IsRun()) {
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
				printf("EasyTcpServer. OnRun. select quit..\n");
				pThread->Exit();
				break;
			}

			// accept
			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);

				this->Accept();
			}
		}
	}

	bool IsRun()
	{
		return INVALID_SOCKET != _sock;
	}

	void TimeForMsg()
	{
		auto t1 = _time.GetElapsedTimeInSec();
		if( t1 >= 1.0){
			printf("thread<%d>, time<%lf>,client nums<%d> recv<%d>,msg<%d>\n", _cellServers.size(), t1, (int)_clientCnt, (int)(_recvCnt / t1), (int)(_msgCnt / t1));
			_msgCnt = 0;
			_recvCnt = 0;
			_time.Update();
		}
	}

	virtual void OnNetLeave(std::shared_ptr<CellClient> client)
	{
		_clientCnt--;
	}

	virtual void OnNetMsg(CellServer* cellServer, std::shared_ptr<CellClient> client, Netmsg_DataHeader* header)
	{
		_msgCnt++;
	}

	virtual void OnNetJoin(std::shared_ptr<CellClient> client)
	{
		_clientCnt++;
	}

	virtual void OnNetRecv(std::shared_ptr<CellClient> client)
	{
		_recvCnt++;
	}

private:
	std::vector<std::shared_ptr<CellServer>> _cellServers;
	CellThread _thread;
	TimeStamp _time;
	SOCKET _sock;

protected:
	std::atomic<int> _msgCnt;
	std::atomic<int> _recvCnt;
	std::atomic<int> _clientCnt;
};