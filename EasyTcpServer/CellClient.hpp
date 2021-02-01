#pragma once

#include "Common.h"

#define CLIENT_HEART_DEAD_TIME 5000

// client class
class CellClient : public IObjectPool<CellClient, 1000>
{
public:
	CellClient(SOCKET sockfd = INVALID_SOCKET)
	{
		_sockfd = sockfd;
		memset(_msgBuf, 0, sizeof(_msgBuf));
		_lastPos = 0;
		memset(_sendBuf, 0, sizeof(_sendBuf));
		_lastSendPos = 0;

		this->RestLife();
	}
	~CellClient()
	{
	}

	SOCKET Sockfd() { return _sockfd; }
	char* MsgBuf() { return _msgBuf; }
	int GetLastPos() { return _lastPos; }
	void SetLastPos(int pos) { _lastPos = pos; }

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

	void RestLife()
	{
		_life = 0;
	}

	bool CheckLife(time_t t)
	{
		_life += t;
		if (_life >= CLIENT_HEART_DEAD_TIME) {
			printf("check life dead: socket=<%d>, time=%d..\n", (int)_sockfd, (int)_life);
			return true;
		}
		return false;
	}

private:
	SOCKET _sockfd; // fd_set file desc set
	char _msgBuf[RECV_BUFF_SIZE * 5] = {}; // the 2nd buffer
	int _lastPos = 0;
	char _sendBuf[SEND_BUFF_SIZE] = {};
	int _lastSendPos = 0;
	time_t _life;
};