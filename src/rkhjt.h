#ifndef _RK_HJT_H_
#define _RK_HJT_H_ 

#include "rktype.h"
#include "rkcrc.h"

#define HJT_MSG_MAX_LEN 1024
#define HJT_MSG_HDR_LEN 6
#define HJT_MSG_SAFE_LEN 950

#define HJT_DATA_ANSWER_BIT 0x01
#define HJT_DATA_NUM_BIT 0x02

typedef enum {
	HJTREQREADYEXEC = 1,
	HJTREQREJECT,
	HJTREQERRORPW,
} HJT_RTN_T;

typedef enum {
	HJTEXECSUCCESS = 1,
	HJTEXECFAILED,
	HJTEXECNODATA,
} HJT_RST_T;

typedef struct hjtPkt {
	char qn[24]; /* Request Number */
	char pw[8]; /* Password */
	char mn[16]; /* Monitor Number */
	uint8_t st; /* System Type */
	uint8_t flag; 
	uint16_t cn; /* Command Number */
	uint16_t pnum; /* Packet Number */
	uint16_t pno; /* Packet NO. */
	char cp[1024]; /* Command Parameter */
} hjtPkt_t;

typedef enum {
	HJTERRMN = 1,
	HJTERRPW,
	HJTERRMSG,
} HJT_ERR_T;

/* 
 * Command number map 
 */
typedef struct hjtCnMap {
	int (* func)(struct hjtPkt *pkt, char *msg, void *arg);
	uint16_t cn;
	int flag; /* if flag is 1, we need answer */
} hjtCnMap_t;

/* Global Constant */
extern const struct hjtCnMap hjtCnTab[];

int rkHjtInit(struct context *handle);
int rkHjtParseMsg(char *msg, struct hjtPkt *pkt);
void rkHjtFillMsgHdr(uint16_t cn, char *buf);
void rkHjtFillMsgEnd(char *msg);
char *rkHjtGenMsgQnRtn(HJT_RTN_T rtn, struct hjtPkt *msg);
void rkHjtFree(void *handle);

/* HJT/212 Functions */
int rkHjtExeCmd1000(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd1001(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg1011(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd1012(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg1021(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd1022(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg1031(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd1032(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg1041(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd1042(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg1061(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd1062(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd1072(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg1091(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd1092(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg2011(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd2012(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg2021(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd2022(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg2031(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg2041(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg2051(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg2061(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg2071(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg2072(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg2081(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd3011(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd3012(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd3013(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtExeCmd3014(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg9011(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg9012(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg9013(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg9014(struct hjtPkt *pkt, char *msg, void *arg);
int rkHjtGenMsg3051(struct hjtPkt *pkt, char *msg, void *arg);

#endif /* _RK_HJT_H_ */
