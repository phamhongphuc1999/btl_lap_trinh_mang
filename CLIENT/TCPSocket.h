#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include <Windows.h>
#include "Config.h"
#include "Common.h"

#pragma comment(lib,"WS2_32")

struct Message {
	int type;
	char fileName[BUFF_SIZE];
	char ID[BUFF_SIZE];
	char data[BUFF_SIZE + 1];
	int dataLen;
};

static int MessageSize = sizeof(Message);

int SEND_TCP(SOCKET s, Message data, int flag);
int RECEIVE_TCP(SOCKET s, Message* data, int flag);
void CreateMessage(Message* message, int type, char* fileName, char* ID, char* data, int dataLen);
