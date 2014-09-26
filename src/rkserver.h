#ifndef RKSERVER
#define RKSERVER

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "rkprotocol.h"

int rkTcpSocketInit();
void rkTcpServerThread(void *arg);
int rkServerSendMsg(rkMsgPkt_t *pkt);
int rkHandleReceivedMsg(rkMsgPkt_t *pkt);
void rkPrintMsg(rkMsgPkt_t *pkt, int recvFlag);
void rkRebootDevice();

#endif //RKSERVER
