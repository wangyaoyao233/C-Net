#pragma once

#include "Common.h"

class CellClient;
class CellServer;

// net event interface
class INetEvent
{
public:
	virtual void OnNetLeave(std::shared_ptr<CellClient> client) = 0;
	virtual void OnNetMsg(CellServer* cellServer, std::shared_ptr<CellClient> client, Netmsg_DataHeader* header) = 0;
	virtual void OnNetJoin(std::shared_ptr<CellClient> client) = 0;
	virtual void OnNetRecv(std::shared_ptr<CellClient> client) = 0;
};