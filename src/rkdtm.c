/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.0
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkdtm.c
 * Created At		: 2013-09-23 10:56
 * Last Modified	: 2013-11-11 13:14
 * Description		: 
 * History			: 
*******************************************************************************/

#include "rkdtm.h"
#include "rkdam.h"
#include "rkdsm.h"
#include "rkser.h"
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

static struct com dtmcom;
static struct net *dtmnet;

static struct {
	int (* send)(const char *msg, uint32_t len);
	int (* recv)(char *msg, uint32_t len, uint32_t timeout_ms);
} dtmcfi;

int rkDtmInit(struct context *ctx)
{
	int ret, rc1, rc2;

	dtmnet = &ctx->m_tNetParam;
	pthread_mutex_init(&dtmnet->mutex, NULL);

	if (IS_MSG_UPLOAD_BY_DTU(ctx->m_tSystemParam.dum)) {
		ret = rkDtmDtuInit(&dtmcom);
		if (ret == -1) {
			fprintf(stderr, "%s : DTU initialize failed.\n", __func__);
			CLEAR_DTU_LINK_FLAG(dtmnet->linkst);
		} else {
			SET_DTU_LINK_FLAG(dtmnet->linkst);
		}

		dtmcfi.send = rkSendMsgByDtu;
		dtmcfi.recv = rkRecvMsgByDtu;
	} else if (IS_MSG_UPLOAD_BY_ETH(ctx->m_tSystemParam.dum)) {
		ret = rkDtmEthInit(dtmnet);
		if (ret == -1) {
			fprintf(stderr, "%s : Ethernet initialize failed.\n", __func__);
			CLEAR_ETH_LINK_FLAG(dtmnet->linkst);
		} else {
			SET_ETH_LINK_FLAG(dtmnet->linkst);
		}

		dtmcfi.send = rkSendMsgByEth;
		dtmcfi.recv = rkRecvMsgByEth;
	} else if (IS_MSG_UPLOAD_BY_ETH_AND_DTU(ctx->m_tSystemParam.dum)) {
		rc1 = rkDtmDtuInit(&dtmcom);
		if (rc1 == -1) {
			fprintf(stderr, "%s : Initialize DTU failed.\n", __func__);
			CLEAR_DTU_LINK_FLAG(dtmnet->linkst);
		} else {
			SET_DTU_LINK_FLAG(dtmnet->linkst);
		}

		rc2 = rkDtmEthInit(dtmnet);
		if (rc2 == -1) {
			fprintf(stderr, "%s : Initialize ehternet failed.\n", __func__);
			CLEAR_ETH_LINK_FLAG(dtmnet->linkst);
		} else {
			SET_ETH_LINK_FLAG(dtmnet->linkst);
		}

		dtmcfi.send = rkSendMsgBoth;
		dtmcfi.recv = rkRecvMsgBoth;

		ret = rc1 < 0 && rc2 < 0 ? -1 : 0;
	}

	rkHjtInit(ctx);

	return ret;
}

int rkDtmCloseDtu()
{
	rkSerClose(dtmcom.fd);
	dtmcom.fd = -1;

	return 0;
}

int rkDtmReuseDtu()
{
	return rkDtmDtuInit(&dtmcom);
}

int rkDtmDtuInit(struct com *com)
{
	com->id = DTU_DEV_ID;
	com->baud = 57600;
	com->db = 8;
	com->sb = 1;
	com->parity = NONE;

	return rkSerOpen(com) == -1 ? -1 : 0;
}

int rkDtmEthInit(struct net *net)
{
	switch(net->cm) {
		case UDPCLIENT:
			return rkUdpClntInit(net);
		case UDPSERVER:
			return rkUdpSrvInit(net);
		case TCPCLIENT:
			return rkTcpClntInit(net);
		case TCPSERVER:
			return rkTcpSrvInit(net);
		default: 
			return -1;
	}

	return 0;
}

int rkUdpClntInit(struct net *net)
{
	int ret;
	struct sockaddr_in addr;
	struct timeval tm_out;

	/* Create socket file descriptor */
	net->connectfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (net->connectfd == -1) {
		return -1;
	}

	/* Reuse IP address */
	ret = 1;
	if (setsockopt(net->connectfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret)) == -1) {
		close(net->connectfd);
		return -1;
	}

	/* Check validity of server address and port */
	if (!net->rport || !strlen(net->raddr)) {
		close(net->connectfd);
		return -1;
	}

	/* Set receive timeout */
	tm_out.tv_sec = DTM_RECV_TIMEOUT_MS / 1000;
	tm_out.tv_usec = (DTM_RECV_TIMEOUT_MS % 1000) * 1000;
	if (setsockopt(net->connectfd, SOL_SOCKET, SO_RCVTIMEO, &tm_out, sizeof(struct timeval)) == -1) {
		close(net->connectfd);
		return -1;
	}

	/* Bind server address and port */
	bzero(&addr, sizeof(addr));
	addr.sin_family = PF_INET;
	addr.sin_port = htons(net->rport);
	addr.sin_addr.s_addr = inet_addr(net->raddr);

	ret = connect(net->connectfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret == -1) {
		close(net->connectfd);
		return -1;
	}

	return 0;
}

int rkUdpSrvInit(struct net *net)
{
	return 0;
}

int rkTcpClntInit(struct net *net)
{
	struct sockaddr_in addr;
	struct timeval tm;
	int ret;

	bzero(&addr, sizeof(addr));
	net->connectfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (net->connectfd == -1) {
		return -1;
	}

	/* Create socket file descriptor */
	ret = 1;
	if (setsockopt(net->connectfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret)) == -1) {
		goto error1;
	}

	/* set send timeout */
	tm.tv_sec = DTM_SEND_TIMEOUT_MS / 1000;
	tm.tv_usec = (DTM_SEND_TIMEOUT_MS % 1000) * 1000;
	if (setsockopt(net->connectfd, SOL_SOCKET, SO_SNDTIMEO, &tm, sizeof(tm)) == -1) {
		goto error1;
	}

	/* set receive timeout */
	tm.tv_sec = DTM_RECV_TIMEOUT_MS / 1000;
	tm.tv_usec = (DTM_RECV_TIMEOUT_MS % 1000) * 1000;
	if (setsockopt(net->connectfd, SOL_SOCKET, SO_RCVTIMEO, &tm, sizeof(tm)) == -1) {
		goto error1;
	}

	addr.sin_family = PF_INET;
	addr.sin_port = htons(net->rport);
	addr.sin_addr.s_addr = inet_addr(net->raddr);
#if 0
	printf("net->raddr = %s, %d.%d.%d.%d\n", net->raddr, 
			addr.sin_addr.s_addr & 0xff,
			addr.sin_addr.s_addr >> 8 & 0xff, 
			addr.sin_addr.s_addr >> 16 & 0xff, 
			addr.sin_addr.s_addr >> 24 & 0xff);
#endif

	ret = connect(net->connectfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret == -1) {
		goto error1;
	}

	ret = setsockopt(net->connectfd, SOL_SOCKET, SO_KEEPALIVE, &ret, sizeof(ret));
	if (ret == -1) {
		goto error1;
	}

	return 0;

error1:
	close(net->connectfd);
	return -1;
}

int rkTcpSrvInit(struct net *net)
{
	return 0;
}

int rkDtmSend(const char *msg, uint32_t len)
{
	return dtmcfi.send(msg, len);
}

int rkDtmRecv(char *msg, uint32_t len, uint32_t timeout_ms)
{
	return dtmcfi.recv(msg, len, timeout_ms);
}

int rkSendMsgByDtu(const char *msg, uint32_t len)
{
	return rkSerSend(msg, len, dtmcom.fd);
}

int rkSendMsgByEth(const char *msg, uint32_t len)
{
	int rc;
	time_t tm;
	char buf[256];

	time(&tm);
	if (IS_ETH_LINK_ABNORMAL(dtmnet->linkst)) {
		return -1;
	}

	pthread_mutex_lock(&dtmnet->mutex);
	rc = send(dtmnet->connectfd, msg, len, 0);
	pthread_mutex_unlock(&dtmnet->mutex);
	sprintf(buf, "%s", ctime(&tm));
	buf[strlen(buf - 1)] = '\0';
	printf("%s : send_count = %d\n", ctime(&tm), rc);
	if (rc == -1) {
		pthread_mutex_lock(&dtmnet->mutex);
		CLEAR_ETH_LINK_FLAG(dtmnet->linkst);
		pthread_mutex_unlock(&dtmnet->mutex);
	}

	return rc;
}

int rkSendBeatPktByEth(void)
{
	const char *pkt = "This Is A Heartbeat Packet!\n\r";
#if 0
	int rc;
	rc = send(dtmnet->connectfd, pkt, strlen(pkt), 0);
	printf("rc = %d\n", rc);
	return rc;
#else
	return send(dtmnet->connectfd, pkt, strlen(pkt), 0);
#endif
}

int rkSendMsgBoth(const char *msg, uint32_t len)
{
	int ret1, ret2;

	ret1 = rkSendMsgByEth(msg, len);
	ret2 = rkSendMsgByDtu(msg, len);

	return ret1 > 0 || ret2 > 0 ? 0 : -1;
}

int rkRecvMsgByDtu(char *msg, uint32_t len, uint32_t timeout_ms)
{
	int trytime, needrecv = len, received = 0;
	int ret;

	for(trytime = len / 10; trytime; trytime--) {
		ret = rkSerRecv(&msg[received], 10, timeout_ms, dtmcom.fd);
		if (ret == -1) return -1;
		needrecv -= ret;
		received += ret;
	}

	if (needrecv > 0) {
		ret = rkSerRecv(&msg[received], needrecv, timeout_ms, dtmcom.fd);
		if (ret == -1) return -1;
		needrecv -= ret;
		received += ret;
	}

	return received;
}

int rkRecvMsgByEth(char *msg, uint32_t len, uint32_t timeout_ms)
{
	struct timeval tm_out;
	int ret;

	if (IS_ETH_LINK_ABNORMAL(dtmnet->linkst)) {
		return -1;
	}

	/* Set receive timeout */
	tm_out.tv_sec = timeout_ms / 1000;
	tm_out.tv_usec = (timeout_ms % 1000) * 1000;
	setsockopt(dtmnet->connectfd, SOL_SOCKET, SO_RCVTIMEO, &tm_out, sizeof(struct timeval));

	ret = read(dtmnet->connectfd, msg, len);
	if (ret == 0) {
		pthread_mutex_lock(&dtmnet->mutex);
		close(dtmnet->connectfd);
		pthread_mutex_unlock(&dtmnet->mutex);
		CLEAR_ETH_LINK_FLAG(dtmnet->linkst);
		return -1;
	}

	return ret;
	//return recv(dtmnet->connectfd, msg, len, 0);
}

int rkRecvMsgBoth(char *msg, uint32_t len, uint32_t timeout_ms)
{
	int ret1, ret2;

	//ret1 = rkSerRecv(msg, len, timeout_ms, dtmcom.fd);
	ret1 = rkRecvMsgByDtu(msg, len, timeout_ms);
	//ret2 = recv(dtmnet->connectfd, msg, len, 0);
	ret2 = rkRecvMsgByEth(msg, len, timeout_ms);

	return ret1 > 0 || ret2 > 0 ? 0 : -1;
}

void rkDtmSigHandler(int sig)
{
	fprintf(stderr, "Ethernet Link Dropped!\n");
	//CLEAR_ETH_LINK_FLAG(dtmnet->linkst);
}

void rkDtmRun(void *handle)
{
	int ret;
	pthread_t monitorThread, cmdThread, uploadThread;

	signal(SIGPIPE, rkDtmSigHandler);

	/* Monitor net link state */
	ret = pthread_create(&monitorThread, NULL, (void *)rkDtmMonitorThread, handle);
	if (ret < 0) {
		fprintf(stderr, "Can't create rkDtmMonitorThread : %s.\n", strerror(errno));
		pthread_exit(&ret);
	}

	ret = pthread_create(&cmdThread, NULL, (void *)rkDtmRecvThread, handle);
	if (ret < 0) {
		fprintf(stderr, "Can't create rkDtmRecvThread : %s.\n", strerror(errno));
		pthread_exit(&ret);
	}

	ret = pthread_create(&uploadThread, NULL, (void *)rkDtmUploadThread, handle);
	if (ret < 0) {
		fprintf(stderr, "Can't create rkDtmUploadThread : %s.\n", strerror(errno));
		pthread_exit(&ret);
	}

	pthread_join(monitorThread, NULL);
	pthread_join(cmdThread, NULL);
	pthread_join(uploadThread, NULL);

	return;
}

void rkDtmMonitorThread(void *handle)
{
	int cnt = 0, trytime;
	static time_t	last_time = 0;
	time_t	current_time;

	struct context *ctx = (struct context *)handle;

	struct timeval timeout;

	timeout.tv_sec = 0;
	timeout.tv_usec = 1;

	while(1) {
		time(&current_time);

		if ((IS_MSG_UPLOAD_BY_ETH(ctx->m_tSystemParam.dum) || IS_MSG_UPLOAD_BY_ETH_AND_DTU(ctx->m_tSystemParam.dum))) {
#if 0
			/* Send Heart-Beat Packet */
			if (IS_ETH_LINKED(dtmnet->linkst) && current_time - last_time > 30) {
				if (rkSendBeatPktByEth() == -1) {
					fprintf(stderr, "Ethernet Link Dropped!\n");
					CLEAR_ETH_LINK_FLAG(dtmnet->linkst);
				}
				last_time = current_time;
			}
#endif
			if (IS_ETH_LINK_ABNORMAL(dtmnet->linkst)) {
				for (trytime = 0; trytime < 3; trytime++) {
					time(&last_time);
					fprintf(stderr, "Ethternet Link Dropped! Reconnecting...(%d)\n", ++cnt);

					if (!rkDtmEthInit(dtmnet)) {
						SET_ETH_LINK_FLAG(dtmnet->linkst);
						fprintf(stderr, "Network Connected!\n");
						break;
					} else {
						CLEAR_ETH_LINK_FLAG(dtmnet->linkst);
					}
					usleep(500000);
				}
			} else {
#if 0
				char recv_buf[16];
				int ret = read(dtmnet->connectfd, recv_buf, sizeof(recv_buf));
				if (ret == 0) {
					pthread_mutex_lock(&dtmnet->mutex);
					close(dtmnet->connectfd);
					pthread_mutex_unlock(&dtmnet->mutex);
					CLEAR_ETH_LINK_FLAG(dtmnet->linkst);
				}
#endif
			}
		}
		usleep(10000);
	}
}

void rkDtmRecvThread(void *handle)
{
	int ret;
	struct hjtMsg msg;

	while(1) {
		ret = rkDtmRecvHjtMsg(&msg, HJT_CMD_RECV_TIMEOUT_MS);
		if (ret == 0) {
			rkDtmProcHjtMsg(&msg);
		}

		usleep(HJT_CMD_RECV_INTERVAL_MS * 1000);
	}

	return;
}

void rkDtmUploadThread(void *handle)
{
	struct context *ctx = (struct context *)handle;
	time_t ctm, ltm = 0; /* Current Time, Last Time */
	struct tm *tms; /* Time Structure */
	int rc, arg, last_month;
	char *msg = NULL;

	time(&ltm);
	while(1) {
		time(&ctm);
		tms = localtime(&ctm);
		/* Upload RTD Message, HJT2011 */
		if (ctx->m_tSystemParam.rduen) {
			if ((ctm - ltm) >= ctx->m_tSystemParam.dui || ctm - ltm < 0) {
				ltm = ctm;
				do {
					rc = rkHjtGenMsg2011(NULL, &msg, NULL);
					if (rc >= 0 && msg != NULL) {
						rkDsmSaveMsg(MSGTYPERTM, msg);
						rkDtmSend(msg, strlen(msg));
						rkHjtFree(msg);
					} else {
						fprintf(stderr, "%s : failed to generate message HJT2011.\n", __func__);
					}
				} while (rc > 0);
			}
		}

		/* Upload MOM Message, HJT2051 */
		if (ctx->m_tSystemParam.mduen && (tms->tm_min % 10 == 0) && (tms->tm_sec == 0)) {
			do {
				rc = rkHjtGenMsg2051(NULL, &msg, NULL);
				//printf("msg = %s\n", msg);
				//printf("strlen(msg) = %d\n", strlen(msg));
				if (rc >= 0 && msg != NULL) {
					rkDsmSaveMsg(MSGTYPEMOM, msg);
					rkDtmSend(msg, strlen(msg));
					rkHjtFree(msg);
				} else {
					fprintf(stderr, "%s : failed to generate message HJT2051.\n", __func__);
				}
			} while(rc > 0);
			rkDamClrStatData(ctx, MNT_STAT_DATA);
		}

		/* Upload HOM Message, HJT2061 */
		if (ctx->m_tSystemParam.hduen  && (tms->tm_min == 0) && (tms->tm_sec == 5)) {
			do {
				rc = rkHjtGenMsg2061(NULL, &msg, NULL);
				if (rc >= 0 && msg != NULL) {
					rkDtmSend(msg, strlen(msg));
					rkDsmSaveMsg(MSGTYPEHOM, msg);
					rkHjtFree(msg);
				} else {
					fprintf(stderr, "%s : failed to generate message HJT2061.\n", __func__);
				}
				rkDamClrStatData(ctx, HOU_STAT_DATA);
			} while(rc > 0);
		}

		/* Upload DOM Message, HJT2031 */
		if (ctx->m_tSystemParam.dduen  && (tms->tm_hour == 0) && (tms->tm_min == 0) && (tms->tm_sec == 10)) {
			do {
				rc = rkHjtGenMsg2031(NULL, &msg, NULL);
				if (rc >= 0 && msg != NULL) {
					rkDtmSend(msg, strlen(msg));
					rkDsmSaveMsg(MSGTYPEDOM, msg);
					rkHjtFree(msg);
				} else {
					fprintf(stderr, "%s : failed to generate message HJT2031.\n", __func__);
				}
				rkDamClrStatData(ctx, DAY_STAT_DATA);
			} while(rc > 0);
		}

		/* Clear Month Statistic Data Monthly */
		if (tms->tm_mon != last_month) {
			last_month = tms->tm_mon;
			rkDamClrStatData(ctx, MON_STAT_DATA);
		}

		/* Upload DI Message, HJT3051*/
		if (ctx->m_tSystemParam.sduen && ctx->m_tDioParam.m_uDiChangedFlag) {
			do {
				rc = rkHjtGenMsg3051(NULL, &msg, NULL);
				if (rc >= 0 && msg != NULL) {
					rkDsmSaveMsg(MSGTYPEDIM, msg);
					rkDtmSend(msg, strlen(msg));
					rkHjtFree(msg);
				} else {
					fprintf(stderr, "%s : failed to generate message HJT3051.\n", __func__);
				}
				ctx->m_tDioParam.m_uDiChangedFlag = 0;
			} while(rc > 0);
		}

		/* Upload Alarm Message, HJT2072 */
		if (ctx->m_tSystemParam.alarmen && ctx->m_tAnalogParam.m_uAlarmFlag) {
			do {
				arg =  ctx->m_tAnalogParam.m_uAlarmFlag == 1 ? ALARM_H : ALARM_N;
				rc = rkHjtGenMsg2072(NULL, &msg, &arg);
				if (rc >= 0 && msg != NULL) {
					rkDsmSaveMsg(MSGTYPEAOM, msg);
					rkDtmSend(msg, strlen(msg));
					rkHjtFree(msg);
				} else {
					fprintf(stderr, "%s : failed to generate message HJT2072.\n", __func__);
				}
				ctx->m_tAnalogParam.m_uAlarmFlag = 0;
			} while(rc > 0);
		}

		/* Reboot System At 24:00:00 */
		if ((tms->tm_hour == 0) && (tms->tm_min == 0) && (tms->tm_sec == 30)) {
			system("reboot");
		}

		sleep(1);
	}

	return;
}

int rkDtmRecvHjtMsg(struct hjtMsg *msg, uint32_t timeout_ms)
{
	char recvbuf[512];
	int ret, len;
	uint16_t crc1, crc2;
	uint32_t hjtRecved = 0;
	HJT_ERR_T err;

	/* Look for packet header */
	while(1) {
		ret = rkDtmRecv(recvbuf, 1, timeout_ms);
		if (ret <= 0) {
			return -1;
		}

		if (recvbuf[0] == '#') {
			hjtRecved = hjtRecved != 1 ? 1 : 2;
			if (hjtRecved == 2) {
				break;
			}
		}  else {
			hjtRecved = 0;
		}
	}

	/* Get packet length */
	bzero(recvbuf, sizeof(recvbuf));
	ret = rkDtmRecv(recvbuf, 4, timeout_ms);
	if (ret != 4) {
		return -1;
	}
	len = atoi(recvbuf) + 6;
	if (len <= 6) {
		return -1;
	}
	ret = rkDtmRecv(recvbuf, len, timeout_ms);
	if (ret != len || recvbuf[len - 2] != '\r' || recvbuf[len - 1] != '\n') {
		return -1;
	}

	recvbuf[len - 2] = '\0';
	sscanf(&recvbuf[len - 6], "%4hX", &crc1);
	crc2 = rkCrc16(recvbuf, len - 6);
	if (crc1 != crc2) {
		return -1;
	}

	ret = rkHjtParseMsg(recvbuf, msg, &err);
	if (ret) {
		switch(err) {
			case HJTERRMN:
				/* 
				 * Do Nothing
				 */
				break;
			case HJTERRPW:
				{
					char *rsp;
					int arg = HJTREQERRORPW;
					rkHjtGenMsg9011(msg, &rsp, &arg);
					rkDtmSend(rsp, strlen(rsp));
					rkHjtFree(rsp);
				} break;
			case HJTERRMSG:
				bzero(msg, sizeof(struct hjtMsg));
				break;
			default:
				break;
		}
		return -1;
	}

	return 0;
}

extern struct hjtCnMap hjtCnTbl[];

int rkDtmProcHjtMsg(struct hjtMsg *msg)
{
	int ret, arg;
	char *rsp = NULL;
	struct hjtCnMap *node;

	if (msg->flag & HJT_DATA_ANSWER_BIT) {
		arg = HJTREQREADYEXEC;
		ret = rkHjtGenMsg9011(msg, &rsp, &arg);
		if (ret == 0) {
			rkDtmSend(rsp, strlen(rsp));
			rkHjtFree(rsp);
		}
	}

	for (node = (struct hjtCnMap *)hjtCnTbl; node->func != NULL; node++) {
		/* Unmatch */
		if (node->cn != msg->cn) {
			continue;
		}

		ret = node->func(msg, &rsp, NULL);

		/* Command Need Response */
		if (node->flag == 1 && ret == 0) {
			rkDtmSend(rsp, strlen(rsp));
			rkHjtFree(rsp);
		}

		/* Answer Request */
		if (ret == 0) {
			arg = HJTEXECSUCCESS;
		} else if (ret == -1) {
			arg = HJTEXECFAILED;
		} else if (ret == -2) {
			arg = HJTEXECNODATA;
		}

		ret = rkHjtGenMsg9012(msg, &rsp, &arg);
		if (ret == 0) {
			rkDtmSend(rsp, strlen(rsp));
			rkHjtFree(rsp);
		}
	}

	return 0;
}
