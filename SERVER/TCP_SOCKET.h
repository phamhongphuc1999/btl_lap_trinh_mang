#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include "CONST.h"
#include "Common.h"

#pragma comment(lib, "Ws2_32.lib")

struct Message {
	int type;
	char data[BUFF_SIZE];
	char fileName[BUFF_SIZE];
	char partnerID[BUFF_SIZE];
};

static int MessageSize = sizeof(Message);

int SEND_TCP(SOCKET s, Message data, int flag);
int RECEIVE_TCP(SOCKET s, Message* data, int flag);
void CreateMessage(Message* message, int type, char* data, char* fileName, char* partnerID);
