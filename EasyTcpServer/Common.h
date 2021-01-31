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
#include <string.h>
#endif // _WIN32

#include "ObjectPool.hpp"
#include "Message.hpp"
#include "TimeStamp.hpp"

#include <iostream>
#include <memory> // std::shared_ptr


#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#define SEND_BUFF_SIZE (10240 * 5)
#endif // !RECV_BUFF_SIZE
