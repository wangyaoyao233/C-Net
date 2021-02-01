#pragma once

#include "Common.h"
#include "INetEvent.hpp"
#include "CellClient.hpp"

#include <vector>
#include <map>
#include <thread> // std::thread
#include <mutex> // std::mutex



class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET)
	{
		_sock = sock;
		_netEvent = nullptr;
	}
	~CellServer()
	{
		Close();
	}

	void Close()
	{
		_taskServer.Close();
		if (INVALID_SOCKET != _sock) {
			_clients.clear();
			_clientsBuffer.clear();
		}
		_sock = INVALID_SOCKET;
	}


	bool OnRun()
	{
		fd_set fdRead_back{};
		int maxSock = 0;

		while (IsRun())
		{
			// get client from clients buffer queue
			if (!_clientsBuffer.empty()) {
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto client : _clientsBuffer) {
					_clients[client->Sockfd()] = client;
				}
				_clientsBuffer.clear();
				_clientChange = true;
			}
			// if no client
			if (_clients.empty()) {
				std::chrono::microseconds t(1);
				std::this_thread::sleep_for(t);

				_oldTime = Time::GetNowInMilliSec();

				continue;
			}

			// select
			fd_set fdRead{};
			FD_ZERO(&fdRead);

			if (_clientChange) {
				_clientChange = false;
				// add socket to fd_set
				maxSock = _clients.begin()->first;
				for (auto iter : _clients) {
					FD_SET(iter.first, &fdRead);
					if (iter.first > maxSock) {
						maxSock = iter.first;
					}
				}
				// back up
				memcpy(&fdRead_back, &fdRead, sizeof(fd_set));
			}
			else {
				memcpy(&fdRead, &fdRead_back, sizeof(fd_set));
			}

			timeval t{ 0,0 };
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
			else if (ret == 0) {
				continue;
			}


			this->ReadData(fdRead);
			this->CheckTime();

		}

	}

	void CheckTime()
	{
		auto now = Time::GetNowInMilliSec();
		auto t = now - _oldTime;
		_oldTime = now;

		for (auto iter = _clients.begin(); iter != _clients.end();) {
			// heart check
			if (iter->second->CheckHeart(t)) {				
				// leave event
				if (_netEvent)
					_netEvent->OnNetLeave(iter->second);

				_clientChange = true;
				auto iterOld = iter;
				iter++;
				_clients.erase(iterOld);
				continue;
			}

			// send time check
			iter->second->CheckSend(t);

			iter++;
		}
	}

	void ReadData(fd_set& fdRead)
	{
#ifdef _WIN32
		for (int i = 0; i < fdRead.fd_count; i++) {
			auto iter = _clients.find(fdRead.fd_array[i]);
			if (iter != _clients.end()) {
				if (-1 == this->RecvData(iter->second)) {
					// leave event
					if (_netEvent)
						_netEvent->OnNetLeave(iter->second);

					_clientChange = true;
					_clients.erase(iter);
				}
			}
			else {
				printf("error..iter == _clients.end()\n");
			}
		}
#else
		std::vector<std::shared_ptr<ClientSocket>> temp;
		for (auto iter : _clients)
		{
			if (FD_ISSET(iter.first, &fdRead)) {
				if (-1 == this->RecvData(iter.second)) {
					// leave event
					if (_netEvent)
						_netEvent->OnNetLeave(iter.second);

					_clientChange = true;
					temp.push_back(iter.second);
				}
			}
		}
		for (auto client : temp) {
			_clients.erase(client->Sockfd());
		}
#endif // _WIN32
	}

	bool IsRun()
	{
		return INVALID_SOCKET != _sock;
	}


	int RecvData(std::shared_ptr<CellClient> client)
	{
		// recvbuf	
		int len = recv(client->Sockfd(), _recvBuf, RECV_BUFF_SIZE, 0);
		if (len <= 0) {
			//printf("client<%d> quit..\n", (int)client->Sockfd());
			return -1;
		}
		_netEvent->OnNetRecv(client);

		// copy to 2nd buffer
		memcpy(client->MsgBuf() + client->GetLastPos(), _recvBuf, len);
		client->SetLastPos(client->GetLastPos() + len);

		while (client->GetLastPos() >= sizeof(Netmsg_DataHeader)) {
			// recvbuf  to header
			Netmsg_DataHeader* header = (Netmsg_DataHeader*)client->MsgBuf();
			if (client->GetLastPos() >= header->dataLen) {
				int size = client->GetLastPos() - header->dataLen;
				this->OnNetMsg(client, header);
				memcpy(client->MsgBuf(), client->MsgBuf() + header->dataLen, size);
				client->SetLastPos(size);
			}
			else {
				break;
			}
		}

		return 0;
	}

	void OnNetMsg(std::shared_ptr<CellClient> client, Netmsg_DataHeader* header)
	{
		_netEvent->OnNetMsg(this, client, header);

	}

	void AddClient(std::shared_ptr<CellClient> client)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_clientsBuffer.push_back(client);
	}

	void Start()
	{
		_thread = std::thread(&CellServer::OnRun, this);
		_taskServer.Start();
	}

	size_t GetClientCount()
	{
		return _clients.size() + _clientsBuffer.size();
	}

	void SetEventObj(INetEvent* e)
	{
		_netEvent = e;
	}

	void AddSendTask(std::shared_ptr<CellClient> client, std::shared_ptr<Netmsg_DataHeader> header)
	{
		_taskServer.AddTask([client, header]() {
			client->SendData(header);
			});
	}

private:
	char _recvBuf[RECV_BUFF_SIZE] = {};
	SOCKET _sock;
	std::map<SOCKET, std::shared_ptr<CellClient>>_clients;
	// clients buffer queue
	std::vector<std::shared_ptr<CellClient>> _clientsBuffer;
	std::mutex _mutex;
	std::thread _thread;
	INetEvent* _netEvent;
	CellTaskServer _taskServer;
	time_t _oldTime = Time::GetNowInMilliSec();
	bool _clientChange = false;
};

