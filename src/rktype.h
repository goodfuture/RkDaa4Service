#ifndef _RK_TYPE_H_
#define _RK_TYPE_H_

#include "rkcom.h"
#include "sqlite3.h"
#include <stdint.h>
#include <sys/time.h>
#include <pthread.h>

#define CORE_VERSION "2.1.3"

#ifndef ON
#define ON 1
#endif

#ifndef OFF
#define OFF 0
#endif

//#define IO_SAMPLE_INTERVAL_MS 900
//#define EI_SAMPLE_INTERVAL_MS 500
#define IO_SAMPLE_INTERVAL_MS 1000
#define EI_SAMPLE_INTERVAL_MS 1000

#define DTU_DEV_ID 0xFF

/* Data Upload Mode */
typedef enum {
	DTU = 0,
	ETH,
	BOTH,
} DUM_T;

/* Message Type */
typedef enum {
	MSGTYPERTM = 0, /* Real-Time Message */
	MSGTYPEMOM,		/* Minute Of Message */
	MSGTYPEHOM,		/* Hour Of Message */
	MSGTYPEDOM,		/* Day Of Message */
	MSGTYPEDIM,		/* Digital Input Message */
	MSGTYPEAOM,		/* Alarm Of Message */
} MSG_TYPE_T;

#define IS_MSG_UPLOAD_BY_DTU(a)			(a == DTU)
#define IS_MSG_UPLOAD_BY_ETH(a)			(a == ETH)
#define IS_MSG_UPLOAD_BY_ETH_AND_DTU(a)	(a == BOTH)

/* System Parameter */
typedef struct sys {
	char		sim[16]; /* SIM Card Number */
	char		mn[16]; /* Device Id */
	char		pw[8]; /* Password */
	uint8_t		st; /* System Type */
	uint16_t	dui; /* Data Upload Interval(sec) */
	DUM_T		dum; /* Data Upload Mode */
	uint16_t	dsi; /* Data Save Interval(sec) */
	uint8_t		rduen; /* Real-time Data Upload Enable */
	uint8_t		mduen; /* Minute Data Upload Enable */
	uint8_t		hduen; /* Hour Data Upload Enable */
	uint8_t		dduen; /* Daily Data Upload Enable */
	uint8_t		sduen; /* Switch Data Upload Enable */
	uint8_t		alarmen; /* Warning Enable */
	uint8_t		rtdfalg; /* Use RTD-Flag Or Not */
} sys_t;

/* Communication Mode */
typedef enum {
	UDPCLIENT = 0,
	UDPSERVER,
	TCPCLIENT,
	TCPSERVER,
} CM_T;

#define ETH_LINK_STATUS_BITMASK	0x01
#define DTU_LINK_STATUS_BITMASK	0x02

#define SET_ETH_LINK_FLAG(a)	(a |= ETH_LINK_STATUS_BITMASK)
#define CLEAR_ETH_LINK_FLAG(a)	(a &= ~ETH_LINK_STATUS_BITMASK)
#define SET_DTU_LINK_FLAG(a)	(a |= DTU_LINK_STATUS_BITMASK)
#define CLEAR_DTU_LINK_FLAG(a)	(a &= ~DTU_LINK_STATUS_BITMASK)

#define IS_ETH_LINKED(a)		(a & ETH_LINK_STATUS_BITMASK)
#define IS_ETH_LINK_ABNORMAL(a)	(!(a & ETH_LINK_STATUS_BITMASK))
#define IS_DTU_LINKED(a)		(a & DTU_LINK_STATUS_BITMASK)
#define IS_DTU_LINK_ABNORMAL(a)	(!(a & DTU_LINK_STATUS_BITMASK))

/* Network Parameter */
typedef struct net {
	char laddr[16]; /* Local Ip Address */
	uint16_t lport; /* Local Port */
	char mask[16]; /* Subnet Mask */
	char gw[16]; /* Gateway */
	char dns[16]; /* Dns */
	char raddr[16];  /* Remote Ip Address */
	uint16_t rport; /* Remote Port */
	CM_T cm; /* Communication Mode */
	int listenfd; /* Socket Used For Listen Client Request */
	int connectfd; /* This Socket fd Was Defined When Link Has Been Connected */
	uint8_t linkst; /* Network Link State : 0 - Normal, 1 - Abnormal */
	pthread_mutex_t mutex;
} net_t;

/* Data Type */
typedef enum {
	INT = 0,
	FLT,
} DT_T;

/* Common Function Interface */
typedef struct cfi {
	int (* init)(void *handle);
	char *(* name)(void);
	int (* run)(void *handle);
} cfi_t;

#define RTD_RAW_VAL_OFFSET 0
#define RTD_CONV_VAL_OFFSET	1

#define MNT_DATA_STAT_OFFSET 0
#define HOU_DATA_STAT_OFFSET 1
#define DAY_DATA_STAT_OFFSET 2
#define MON_DATA_STAT_OFFSET 3

typedef enum {
	MNT_STAT_DATA = 0, /* Minute */
	HOU_STAT_DATA, /* Hour */
	DAY_STAT_DATA, /* Day */
	MON_STAT_DATA, /* Month */
} STAT_DATA_TYPE;

/* Data Statistic Information */
typedef struct statinfo{
	long cnt[4];
	float min[4];
	float max[4];
	float avg[4];
	double sum[4];
	double cou[4];
} statinfo_t;

/* External Device Information */
typedef struct edev {
	uint8_t id; /* Channel Id */
	uint8_t inuse; /* If 0 Unuse, Otherwise Inuse */
	char code[8]; /* Channel code */
	float vals[2]; /* Values */
	uint8_t com; /* Uart Id : 0, 1, 2 ... */
	int fd;
	int devid; /* Device Address */
	int regaddr; /* Register Address */
	DT_T datatype; /* Data Type */
	uint8_t isconv; /* Is Convert Value? */
	//uint8_t	useflag; /* Use Data Flag ? */
	uint8_t flag; /* When Data Acquisition Successful This Flag Been Set To 'N', Otherwise 'D' */
	uint8_t usefml; /* Use Formula ? */
	char fml[64]; /* Formula */
	uint8_t alarm;
	float	ulv;
	float	llv;
	float	utv;
	float	ltv;
	pthread_mutex_t mutex;
	struct statinfo stat;
} edev_t;

/* Parity Type */
typedef enum {
	NONE = 0,
	ODD,
	EVEN,
} PARITY_T;

/* Com Port Information */
typedef struct com {
	uint8_t id; /* COM Id */
	char proto[32]; /* Library File Name */
	uint32_t baud;
	uint8_t db; /* Data Bits */
	uint8_t sb; /* Stop Bits */
	PARITY_T parity; /* Parity Mode : ODD, EVEN or NONE */
	int fd; /* Uart Device File Descriptor */
	struct cfi cfi;
} com_t;

/* Uart Parameter */
typedef struct uart {
	struct com	m_tComParam[COM_NUM];
	struct edev m_tChannelParam[EI_NUM]; /* External Instrument */
} uart_t;

/* AI Type */
typedef enum {
	MA20 = 0,
	V5,
	V10,
} AI_T;

/* Digital Input & Output Parameter */
typedef struct dioparam {
	uint8_t id;
	uint8_t inuse;
	char code[8];
	uint8_t val;
	pthread_mutex_t lock;
} dioparam_t;

/* Digital Input & Output Parameter */
typedef struct dio {
	struct dioparam m_tDiParam[DI_NUM];
	struct dioparam m_tDoParam[DO_NUM];
	uint8_t			m_uDiChangedFlag; /* This Flag Been Set When DI Changed */
} dio_t;

/* Analog Input Parameter */
typedef struct aiparam {
	uint8_t id; /* Channel id */
	uint8_t inuse;
	char code[8]; 
	float vals[2]; /* Values */
	AI_T type; /* Analog Variable Type, mA or V */
	float ulv; /* Upper Limit Value */
	float llv; /* Lower Limit Value */
	float utv; /* Upper Threshold Value */
	float ltv; /* Lower Threshold Value */
	uint8_t isconv;
	uint8_t flag; /* Data Acquisition Wether Normal Or Not */
	uint8_t usefml; /* Use Formula */
	char fml[64];  /* Formula */
	uint8_t alarm;
	struct statinfo stat;
	pthread_mutex_t lock;
} aiparam_t;

/* AI Calibrate Parameter */
typedef struct aical {
	uint16_t loffset; /* Low Offset */
	uint16_t hoffset; /* High Offset */
} aical_t;

/* Analog Parameter */
typedef struct analog {
	struct aiparam	m_tChannelParam[AI_NUM];
	struct aical	m_tCalibrateParam[AI_NUM];
	uint8_t			m_uAlarmFlag;
} analog_t;

#define ARG_CFG_DIR_OFFSET 0
#define ARG_DYN_LIB_OFFSET 1
#define ARG_RTD_DB_PATH_OFFSET 2
#define ARG_MSG_DB_PATH_OFFSET 3

#define MAX_CODE_TBL_NUM 10
/* Code-Value Map */
typedef struct codetbl{
	char *m_pCode[MAX_CODE_TBL_NUM];
	float m_fValue[MAX_CODE_TBL_NUM];
} codetbl_t;

#define MAX_CMDLINE_ARG_NUM 5

/* Global Used */
typedef struct context {
	struct sys		m_tSystemParam;
	struct net		m_tNetParam;
	struct uart		m_tUartParam;
	struct dio		m_tDioParam;
	struct analog	m_tAnalogParam;
	struct codetbl	m_tCodeTable;
	int				m_nServerSockFd;
	char			*m_pCmdLineArg[MAX_CMDLINE_ARG_NUM]; 
	uint8_t			debug; /* Debug Flag */
	uint8_t			exit;
	uint8_t			m_uLoginFlag;
	char			m_aVersion[3][8];
	char			m_aDynamicProtocolSet[512];
} context_t;

extern struct context ctx;

#endif /* _RK_TYPE_H_ */
