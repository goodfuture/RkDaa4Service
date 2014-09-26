
#include "rkserver.h"
#include <linux/tcp.h>

extern struct context ctx;
static int connectfd;

int rkTcpSocketInit()
{
	int sockfd, value;
	struct sockaddr_in addr;

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("Failed To Create Socket!");
		return -1;
	}

	value = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
	//Don't Use Nagle Algorithm
	value = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
	//Close System Copy Progress
	value = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &value, sizeof(value));
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(8088);

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr))) {
		perror("Can't Bind TcpSocket!");
		return -1;
	}

	if (listen(sockfd, 5)) {
		perror("Failed To Listen Socket!");
		return -1;
	}

	ctx.m_nServerSockFd = sockfd;

	return 0;
}

void rkTcpServerThread(void *arg)
{
	int fd, pos = 0, ret;
	struct sockaddr_in addr;
	size_t addr_len = sizeof(addr);
	struct rkMsgPkt recvBuffer;

	while((fd = accept(ctx.m_nServerSockFd, (struct sockaddr *)&addr, &addr_len)) >= 0) {
		printf("Accept From Client: %s:%d\n", inet_ntoa(addr.sin_addr), addr.sin_port);
		connectfd = fd;
		do {
			ret = read(connectfd, &recvBuffer.header[pos], 1);
			if (ret == 0) {
				close(connectfd);
				break;
			} else if (ret < 0) {
				continue;
			} else {
				if (pos == 0 && recvBuffer.header[0] == (RK_PROTOCOL_HEADER >> 16)) {
					pos = 1;
					continue;
				} else if (pos == 1 && recvBuffer.header[1] == (RK_PROTOCOL_HEADER >> 8 & 0xFF)) {
					pos = 2;
					continue;
				} else if (pos == 2 && recvBuffer.header[2] == (RK_PROTOCOL_HEADER & 0xFF)) {
					pos = 0;
					if (read(connectfd, &recvBuffer.cw, 1) != 1) {
						continue;
					}

					if (read(connectfd, &recvBuffer.len, 2) != 2) {
						continue;
					}

					recvBuffer.len = ntohs(recvBuffer.len);

					if (read(connectfd, recvBuffer.msg, recvBuffer.len) != recvBuffer.len) {
						continue;
					}

					rkHandleReceivedMsg(&recvBuffer);
				}
			}

			if (ctx.m_uLoginFlag == 0) {
				continue;
			}
		} while(1);
	}

	perror("Failed To Accept!");
}

int rkServerSendMsg(rkMsgPkt_t *pkt)
{
#ifdef DEBUG_SEND_MESSAGE_RAW_OUTPUT 
	rkPrintMsg(pkt, 0);
#endif
	uint16_t len = pkt->len;
	pkt->len = htons(len);

	return write(connectfd, pkt, RK_PROTOCOL_HEADER_LEN + len);
}

int rkHandleReceivedMsg(rkMsgPkt_t *recvBuffer) 
{
	int ret;
	struct rkMsgPkt sendBuffer;

#ifdef DEBUG_RECV_MESSAGE_RAW_OUTPUT
	rkPrintMsg(recvBuffer, 1);
#endif

	if (rkVerifyMsgTail(recvBuffer)) {
		switch(recvBuffer->cw) {
			case CW_USER_LOGIN:
				rkGenEchoMsgOfLogin(recvBuffer, &sendBuffer);
				rkServerSendMsg(&sendBuffer);
				break;
			case CW_GET_PARAM_CONFIG:
				do {
					ret = rkGenEchoMsgOfGetConfig(recvBuffer, &sendBuffer);
					rkServerSendMsg(&sendBuffer);
				} while(!ret);
				break;
			case CW_PUT_PARAM_CONFIG:
				do {
					ret = rkGenEchoMsgOfPutConfig(recvBuffer, &sendBuffer);
					rkServerSendMsg(&sendBuffer);
				} while(!ret);
				break;
			case CW_REBOOT_DEVICE:
				rkGenEchoMsgOfRebootDevice(&sendBuffer);
				rkServerSendMsg(&sendBuffer);
				rkRebootDevice();
				break;
			case CW_UPGRADE_HMI_FIRMWARE:
				rkGenEchoMsgOfUpgradeFirmware(0, recvBuffer, &sendBuffer);
				rkServerSendMsg(&sendBuffer);
				break;
			case CW_UPGRADE_BLP_FIRMWARE:
				rkGenEchoMsgOfUpgradeFirmware(1, recvBuffer, &sendBuffer);
				rkServerSendMsg(&sendBuffer);
				break;
			case CW_UPGRADE_DTU_TOOL_FIRMWARE:
				rkGenEchoMsgOfUpgradeFirmware(2, recvBuffer, &sendBuffer);
				rkServerSendMsg(&sendBuffer);
				break;
			case CW_GET_FIRMWARE_VERSION:
				rkGenEchoMsgOfGetFirmwareVersion(recvBuffer, &sendBuffer);
				rkServerSendMsg(&sendBuffer);
				break;
			case CW_GET_SUPPORTED_PROTOCOL:
				rkGenEchoMsgOfGetSupportedProtocol(&sendBuffer);
				rkServerSendMsg(&sendBuffer);
				break;
			default:
				break;
		}
	}

	return 1;
}

void rkPrintMsg(rkMsgPkt_t *pkt, int recvFlag)
{
	int index = 0;

	if (recvFlag) {
		printf("\033[40;31mRecv : %d\n", pkt->len + RK_PROTOCOL_HEADER_LEN);
	} else {
		printf("\033[40;32mSend : %d\n", pkt->len + RK_PROTOCOL_HEADER_LEN);
	}
	printf("%02X ", pkt->header[0]);
	printf("%02X ", pkt->header[1]);
	printf("%02X ", pkt->header[2]);
	printf("%02X ", pkt->cw);
	printf("%02X ", pkt->len >> 8);
	printf("%02X ", pkt->len & 0xFF);
	for (index = 0; index < pkt->len; index++) {
		printf("%02X ", pkt->msg[index]);
	}
	printf("\033[0m\n");
}

void rkRebootDevice()
{
	system("reboot");
}
