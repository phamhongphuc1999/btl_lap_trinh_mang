// SERVER.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "TCP_SOCKET.h"
#include "Common.h"
#include "CONST.h"
using namespace std;

unsigned _stdcall Handler(void* param);

//list of session
list<SESSION*> listSession;
int lockSession, isThreadFull;

struct ForwardInfo {
	SOCKET client;
	char* ID;
};

struct SearchInfo {
	SOCKET client;
	char* ID;
	char* fileName;
};

int main()
{
	WSADATA wsaData;
	WORD version = MAKEWORD(2, 2);
	if (WSAStartup(version, &wsaData)) {
		printf("Version is not supported\n");
		return 0;
	}
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	u_short serverPort = 0;
node1:
	printf("%s ", SERVER_EXE);
	scanf_s("%d", &serverPort);
	if (serverPort < 1) {
		printf("Wrong port\n");
		goto node1;
	}
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr))) {
		printf("Can not bind this address"); 
		return 0;
	}
	if (listen(listenSocket, MAX_CLIENT)) {
		printf("Can not listen"); 
		return 0;
	}
	printf("SERVER STARTED\n");
	lockSession = 0; isThreadFull = 1;
	while (true)
	{
		if (isThreadFull == 1) {
			_beginthreadex(0, 0, Handler, (void*)listenSocket, 0, 0);
			isThreadFull = 0;
		}
	}
}

#pragma region HANDLER MULTIPLE CLIENT
//Add session in listSession
unsigned _stdcall CreateSession(void* param) {
	while (true)
	{
		if (lockSession == 0) {
			lockSession = 1;
			SESSION* session = (SESSION*)param;
			listSession.push_back(session);
			session->position = --listSession.end();
			lockSession = 0;
			break;
		}
	}
	return 0;
}

//Remove session from listSession
unsigned _stdcall ReleaseSession(void* param) {
	while (true)
	{
		if (lockSession == 0) {
			lockSession = 1;
			SESSION* session = (SESSION*)param;
			listSession.erase(session->position);
			lockSession = 0;
			break;
		}
	}
	return 0;
}

unsigned _stdcall ForwardFile(void* param) {
	ForwardInfo* info = (ForwardInfo*)param;
	char* data = new char[BUFF_SIZE];
	char* fileName = new char[BUFF_SIZE];
	char* opcode = new char[10];
	bool check = false;
	SESSION session;
	for (SESSION* item : listSession) 
		if (!strcmp(info->ID, item->ID)) {
			check = true;
			session = *item;
			break;
		}
	if (check) {
		int ret = SEND_TCP(session.connSock, o_200, info->ID, 0);
		if (ret == SOCKET_ERROR) printf("Can not send from client[%s]\n", session.ID);
		ret = RECEIVE_TCP(session.connSock, opcode, data, 0);
		if (ret == SOCKET_ERROR) printf("Can not receive from client[%s]\n", session.ID);
		if (!strcmp(opcode, o_410)) {
			int ret = SEND_TCP(info->client, o_203, new char[1]{ 0 }, 0);
			if (ret == SOCKET_ERROR) printf("Can not send from client[%s]\n", info->ID);
		}
		else {
			list<char*> payload;
			ret = SEND_TCP(info->client, o_202, new char[1]{ 0 }, 0);
			if (ret == SOCKET_ERROR) printf("Can not send from client[%s]\n", info->ID);
			ret = RECEIVE_TCP(info->client, opcode, fileName, 0);
			if (ret == SOCKET_ERROR) printf("Can not receive to client[%s]\n", info->ID);
			fileName[ret] = 0;
			while (true)
			{
				ret = RECEIVE_TCP(info->client, opcode, data, 0);
				if (ret == SOCKET_ERROR) printf("Can not receive to client[%s]\n", info->ID);
				data[ret] = 0;
				if (!strcmp(data, "")) break;
				payload.push_back(data);
			}
			printf("RECEIVE FINISH\n");
		}
	}
	else {
		int ret = SEND_TCP(info->client, o_203, new char[1]{ 0 }, 0);
		if (ret == SOCKET_ERROR) printf("Can not send from client[%s]\n", info->ID);
	}
	return 0;
}

unsigned _stdcall SearchFile(void* param) {
	SearchInfo* info = (SearchInfo*)param;
	char* opcode = new char[10];
	char* data = new char[BUFF_SIZE];
	list<char*> lClient;
	int ret;
	for (SESSION* item : listSession) {
		if (strcmp(item->ID, info->ID)) {
			ret = SEND_TCP(item->connSock, o_120, info->fileName, 0);
			if (ret == SOCKET_ERROR) continue;
		}
	}
	for (SESSION* item : listSession) {
		if (strcmp(item->ID, info->ID)) {
			ret = RECEIVE_TCP(item->connSock, opcode, data, 0);
			if (ret == SOCKET_ERROR) continue;
			if (!strcmp(opcode, o_321)) lClient.push_back(item->ID);
		}
	}

	return 0;
}

unsigned _stdcall Handler(void* param) {
	SOCKET listenSocket = (SOCKET)param;
	sockaddr_in clientAddr;
	char buffReceive[BUFF_SIZE], buffSend[BUFF_SIZE], opcode[BUFF_SIZE];
	char username[2048], password[2048];
	char* result = new char[10];
	int ret, clientAddrLen = sizeof(clientAddr);
	DWORD nEvents = MAKEWORD(0, 0), i, index;
	SOCKET connSock;

	SESSION client[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT events[WSA_MAXIMUM_WAIT_EVENTS];
	events[0] = WSACreateEvent();
	WSANETWORKEVENTS sockEvent;

	for (int i = 0; i < WSA_MAXIMUM_WAIT_EVENTS; i++) InitiateSession(&client[i]);
	client[0].connSock = listenSocket;
	WSAEventSelect(client[0].connSock, events[0], FD_ACCEPT | FD_CLOSE);
	nEvents++;

	while (true)
	{
		index = WSAWaitForMultipleEvents(nEvents, events, FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED) break;
		index = index - WSA_WAIT_EVENT_0;
		if (index != WSA_WAIT_FAILED && index != WSA_WAIT_TIMEOUT) {
			WSAEnumNetworkEvents(client[index].connSock, events[index], &sockEvent);

			if (sockEvent.lNetworkEvents & FD_ACCEPT) {
				if (sockEvent.iErrorCode[FD_ACCEPT_BIT] != 0) {
					printf("FD_ACCEPT failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
					break;
				}
				if ((connSock = accept(client[index].connSock, (sockaddr *)&clientAddr, &clientAddrLen)) == SOCKET_ERROR) {
					printf("accept() failed with error %d\n", WSAGetLastError());
					continue;
				}
				if (nEvents == WSA_MAXIMUM_WAIT_EVENTS) isThreadFull = 1;
				else {
					for (int j = 1; j < WSA_MAXIMUM_WAIT_EVENTS; j++)
						if (client[j].connSock == 0) {
							char* tempId = CreateRamdomID();
							while (!CheckRamdomID(listSession, tempId))
								tempId = CreateRamdomID();
							client[j].connSock = connSock;
							client[j].ID = tempId;
							events[j] = WSACreateEvent();
							WSAEventSelect(client[j].connSock, events[j], FD_READ | FD_CLOSE);
							HANDLE hCrSession = (HANDLE)_beginthreadex(0, 0, CreateSession, (void*)&client[j], 0, 0);
							WaitForSingleObject(hCrSession, INFINITE);
							nEvents++;
							break;
						}
				}
				WSAResetEvent(events[index]);
				continue;
			}

			if (sockEvent.lNetworkEvents & FD_READ) {
				if (sockEvent.iErrorCode[FD_READ_BIT] != 0) {
					printf("FD_READ failed with error %d\n", sockEvent.iErrorCode[FD_READ_BIT]);
					break;
				}
				ret = RECEIVE_TCP(client[index].connSock, opcode, buffReceive, 0);
				if (ret == SOCKET_ERROR) continue;
				buffReceive[ret] = 0;
				if (!strcmp(opcode, o_300)) {
					ret = SEND_TCP(client[index].connSock, o_100, client[index].ID, 0);
					if (ret == SOCKET_ERROR) printf("Cause error\n");
				}
				else if (!strcmp(opcode, o_400)) {
					ForwardInfo info; 
					info.client = client[index].connSock;
					info.ID = new char[BUFF_SIZE];
					strcpy_s(info.ID, strlen(buffReceive) + 1, buffReceive);
					_beginthreadex(0, 0, ForwardFile, (void*)&info, 0, 0);
				}
				else if (!strcmp(opcode, o_310)) {
					SearchInfo info;
					info.client = client[index].connSock;
					info.ID = new char[BUFF_SIZE];
					info.fileName = new char[BUFF_SIZE];
					strcpy_s(info.ID, strlen(client[index].ID) + 1, client[index].ID);
					strcpy_s(info.fileName, strlen(buffReceive) + 1, buffReceive);
					_beginthreadex(0, 0, SearchFile, (void*)&info, 0, 0);
				}
					
				continue;
			}

			if (sockEvent.lNetworkEvents & FD_WRITE) {
				if (sockEvent.iErrorCode[FD_WRITE_BIT] != 0) {
					printf("FD_WRITE failed with error %d\n", sockEvent.iErrorCode[FD_WRITE_BIT]);
					break;
				}
				continue;
			}

			if (sockEvent.lNetworkEvents & FD_CLOSE) {
				if (sockEvent.iErrorCode[FD_CLOSE_BIT] != 0) printf("Connection shutdown\n");
				else printf("Client close connection\n");

				HANDLE hRelease = (HANDLE)_beginthreadex(0, 0, ReleaseSession, (void*)&client[index], 0, 0);
				WaitForSingleObject(hRelease, INFINITE);
				closesocket(client[index].connSock);
				InitiateSession(&client[index]);
				WSACloseEvent(events[index]);
				nEvents--; 
				continue;
			}
		}
	}
	return 0;
}
#pragma endregion
