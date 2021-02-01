#pragma once

#include "Common.h"

#define CLIENT_HEART_DEAD_TIME 60000 // 60s
#define CLIENT_SEND_BUFF_TIME 200

// client class
class CellClient : public IObjectPool<CellClient, 1000>
{
public:
	int id = -1;
	int serverid = -1;

public:
	CellClient(SOCKET sockfd = INVALID_SOCKET)
	{
		static int n = 1;
		id = n++;

		_sockfd = sockfd;
		memset(_msgBuf, 0, sizeof(_msgBuf));
		_lastPos = 0;
		memset(_sendBuf, 0, sizeof(_sendBuf));
		_lastSendPos = 0;

		this->RestDTHeart();
	}
	~CellClient()
	{
		printf("s=%d, CellClient %d close 1\n",serverid, id);

		if (INVALID_SOCKET != _sockfd) {
#ifdef _WIN32
			closesocket(_sockfd);
#else
			close(_sockfd);
#endif // _WIN32
		}
	}

	SOCKET Sockfd() { return _sockfd; }
	char* MsgBuf() { return _msgBuf; }
	int GetLastPos() { return _lastPos; }
	void SetLastPos(int pos) { _lastPos = pos; }

	int SendDataImmediate(std::shared_ptr<Netmsg_DataHeader> header)
	{
		this->SendData(header);
		this->SendDataImmediate();
	}

	// 立即将发送缓冲区数据发送给客户端
	int SendDataImmediate()
	{
		int ret = SOCKET_ERROR;
		if (_lastSendPos > 0 && SOCKET_ERROR != _sockfd) {
			ret = send(_sockfd, _sendBuf, _lastSendPos, 0);
			_lastSendPos = 0;
			this->RestDTSend();// reset send time
		}
		return ret;
	}

	int SendData(std::shared_ptr<Netmsg_DataHeader> header)
	{
		int ret = SOCKET_ERROR;
		int len = header->dataLen;
		const char* sendData = (const char*)header.get();

		while (true)
		{
			// 定量
			if (_lastSendPos + len >= SEND_BUFF_SIZE) {
				int copyLen = SEND_BUFF_SIZE - _lastSendPos;
				memcpy(_sendBuf + _lastSendPos, sendData, copyLen);
				sendData += copyLen;
				len -= copyLen;
				ret = send(_sockfd, _sendBuf, SEND_BUFF_SIZE, 0);
				_lastSendPos = 0;

				this->RestDTSend(); // reset send time

				if (SOCKET_ERROR == ret)
					break;
			}
			else {
				memcpy(_sendBuf + _lastSendPos, sendData, len);
				_lastSendPos += len;
				break;
			}
		}

		return ret;
	}

	void RestDTHeart()
	{
		_dtHeart = 0;
	}

	void RestDTSend()
	{
		_dtSend = 0;
	}

	bool CheckHeart(time_t t)
	{
		_dtHeart += t;
		if (_dtHeart >= CLIENT_HEART_DEAD_TIME) {
			printf("check heart dead: socket=<%d>, time=%d..\n", (int)_sockfd, (int)_dtHeart);
			return true;
		}
		return false;
	}

	void CheckSend(time_t t)
	{
		_dtSend += t;
		if (_dtSend >= CLIENT_SEND_BUFF_TIME) {
			//printf("check send: socket=<%d>, time=%d..\n", (int)_sockfd, (int)_dtSend);
			// 立即发送缓冲区数据
			this->SendDataImmediate();
			// 重置发送计时
			this->RestDTSend();
		}
	}

private:
	SOCKET _sockfd; // fd_set file desc set
	char _msgBuf[RECV_BUFF_SIZE * 5] = {}; // the 2nd buffer
	int _lastPos = 0;
	char _sendBuf[SEND_BUFF_SIZE] = {};
	int _lastSendPos = 0;
	time_t _dtHeart;
	time_t _dtSend;// last send msg time
};