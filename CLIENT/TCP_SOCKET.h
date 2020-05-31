#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <conio.h>
#include <stdio.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include "CONST.h"

#pragma comment(lib, "Ws2_32.lib")

struct RECEIVE_INFO;
struct SEND_INFO;

char* EncapsulateData(char* opcode, char* data);
int DecapsulationData(char* buff, char* opcode);
int RECEIVE_TCP(SOCKET s, char* opcode, char* buff, int flag);
int SEND_TCP(SOCKET s, char* buff, int flag);
unsigned _stdcall HANDLER_RECEIVE_TCP(void* param);
unsigned _stdcall HANDLER_SEND_TCP(void* param);
void RECEIVE_THREAD_TCP(RECEIVE_INFO* param);
void SEND_THREAD_TCP(SEND_INFO* param);
