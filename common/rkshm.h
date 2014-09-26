/*******************************************************************************
 * Copyright(C)	    : 2013 Rockontrol Industrial Park, Inc.
 * Version          : V1.1
 * Author           : KangQi
 * Tel              : 18636181581/6581
 * Email            : goodfuturemail@gmail.com
 * File Name        : rkshm.h
 * Created At       : 2013-11-11 10:24
 * Last Modified    : 2013-11-18 16:05
 * Description      : 
 * History	        : 
*******************************************************************************/
#ifndef _RK_SHARE_MEM_H_
#define _RK_SHARE_MEM_H_

#include "rkcom.h"
#include <stdint.h>
#include <time.h>

#define DATA_SIZE (4096) /* 4K */
#define FIFO_SIZE (2048) /* 2K */
#define FIFO_OFFSET DATA_SIZE
#define SHM_SIZE (DATA_SIZE + FIFO_SIZE) /* 6K */

#define SHM_DEV_FILE "/rkShm"

typedef enum {
	TYPE_AI,
	TYPE_EI,
	TYPE_DI,
	TYPE_DO,
} SHM_TYPE;

/* 查询类型 */
typedef enum {
	MSG_QURY = 0,
	SER_QURY,
} QUERY_T;

/* 
 *	RTV -- Real Time Value 
 *	TAV -- Ten Minutes Average Value
 *	HAV -- Hour Average Value
 *	DAV -- Day Average Value 
 *	MAV -- Month Average Value
 */

#define RTV_OFFSET 0
#define TAV_OFFSET 1
#define HAV_OFFSET 2
#define DAV_OFFSET 3
#define MAV_OFFSET 4

typedef struct rtdInfo{
	uint8_t id;
	uint8_t inuse;
	char code[8];
	double vals[5];
	ALARM_T alm;
} rtdInfo_t;

typedef struct dioInfo {
	uint8_t id;
	uint8_t inuse;
	char code[8];
	uint8_t val;
} dioInfo_t;

typedef struct queryCond {
	time_t start;
	time_t end;
	uint8_t cmd[64];
	uint8_t type;
	uint8_t reserve[2];
} queryCond_t;

typedef struct queryReq {
	struct queryCond cond;
	unsigned int id;
	uint8_t ask; /* Client Request Flag */
	uint8_t ready; /* Server Ready To Transfer Data */
	uint8_t reserve[2];
} queryReq_t;

#define RELOAD_USR_CFG_BIT_MASK		0x01
#define DISABLE_DTU_BIT_MASK		0x02
#define CORE_PROG_EXIT_BIT_MASK		0x04

typedef struct shmMap {
	volatile uint8_t		flag;
	volatile uint8_t		heartbeat;
	char		version[2][8];
	rtdInfo_t	ai[AI_NUM];
	rtdInfo_t	ei[EI_NUM];
	dioInfo_t	dip[DI_NUM];
	dioInfo_t	dop[DO_NUM];
	queryReq_t	dataReq;
} shmMap_t;

typedef struct rkRec{
	char buf[256];
	unsigned int len;
	struct rkRec *next;
} rkRec_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize Share Memory */
int rkShmInit();

/* Delete Share Memory */
int rkShmDel();

void rkSetReloadFlag();

void rkClearReloadFlag();

int rkIsReloadFlagBeenSet();

int rkIsDtuDisabled();

int rkIsReqExit();

void rkReqCoreExit();

void rkClearExitReq();

void rkDisableDtu(void);

void rkEnableDtu(void);

int rkCoreBeatOnce(const char *ver);

int rkIsCoreAlive();

void rkPutCoreVersion(const char *version);

void rkPutGuiVersion(const char *version);

const char *rkGetGuiVersion();

const char *rkGetCoreVersion();

/* Set Data */
int rkShmPushData(uint8_t type, uint8_t index, void *val);

/* Get Data */
int rkShmGetData(uint8_t type, uint8_t index, void *val);

/* FIFO In */
int rkShmPutDataByFifo(char *data, unsigned int len);

/* FIFO Out */
int rkShmGetDataByFifo(char *buf, unsigned int size);

/* Query History Record, Used For Client Point */
int rkShmQuery(queryCond_t *cond, rkRec_t **result);

/* Clear Query Result, When We Finish Query, We Must Free Record Space, Otherwise Memory Will Leak, Used Form Client Point */
void rkShmClrResult(rkRec_t *record);

/* Listen Query Request, Used For Server Point */
int rkShmListenReq(struct queryCond *cond);

/* Push Query Result To Share Memory, Used For Server Point */
void rkShmEchoReq(char *result, int size);

#ifdef __cplusplus
}
#endif

#endif /* _RK_SHARE_MEM_H_ */
