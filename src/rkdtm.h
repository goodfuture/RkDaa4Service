#ifndef _RK_DTM_H_ /* DTM - Data Transmit Module */
#define _RK_DTM_H_

#include "rktype.h"
#include "rkhjt.h"

#define DTM_SEND_TIMEOUT_MS 1000
#define DTM_RECV_TIMEOUT_MS 1000

#define HJT_CMD_RECV_INTERVAL_MS 1000
#define HJT_CMD_RECV_TIMEOUT_MS 0

/* 
 *  Function : data transmit module initialize.
 *  Param :
 *  Ret : return 0 if success, return -1 if failed.
 */
int rkDtmInit(struct context *ctx);
int rkDtmDtuInit(struct com *com);
int rkDtmEthInit(struct net *net);
void rkDtmSigHandler(int sig);
void rkDtmRun(void *handle);
int rkDtmSend(const char *msg, uint32_t len);
int rkDtmRecv(char *msg, uint32_t len, uint32_t timeout_ms);
int rkDtmRecvHjtMsg(char *msg);
int rkDtmParseHjtMsg(char *msg, struct hjtPkt *pkt);
int rkDtmProcHjtReq(struct hjtPkt *pkt);
int rkDtmCloseDtu();
int rkDtmReuseDtu();

//int rkDtuInit(struct com *com);
int rkUdpClntInit(struct net *net);
int rkUdpSrvInit(struct net *net);
int rkTcpClntInit(struct net *net);
int rkTcpSrvInit(struct net *net);
int rkEthInit(struct net *net);

int rkSendMsgByDtu(const char *msg, uint32_t len);
int rkSendMsgByEth(const char *msg, uint32_t len);
int rkSendBeatPktByEth(void);
int rkSendMsgBoth(const char *msg, uint32_t len);
int rkRecvMsgByDtu(char *msg, uint32_t len, uint32_t timeout_ms);
int rkRecvMsgByEth(char *msg, uint32_t len, uint32_t timeout_ms);
int rkRecvMsgBoth(char *msg, uint32_t len, uint32_t timeout_ms);
void rkDtmRecvThread(void *handle);
void rkDtmUploadThread(void *handle);
void rkDtmMonitorThread(void *handle);


#endif /* _RK_DTM_H_ */
