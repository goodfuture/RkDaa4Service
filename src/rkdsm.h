#ifndef _RK_DSM_H_ /* DSM : Data Storage Module */
#define _RK_DSM_H_

#include "rktype.h"
#include "sqlite3.h" 
#include <time.h>

/* 
 * Abbreviation :
 * Rtd - Real Time Data 
 * Rtm - Real Time Message 
 * Mom - Minutes Of Message
 * Hom - Hours Of Message
 * Dom - Days Of Message
 * Dim - DI(switch) Message
 * AOM - Alarm Of Message
 * RI - Runtime Information
 */

#define RTD_TBL_NAME "RtdTable"
#define RI_TBL_NAME "RITable"
#define RTM_TBL_NAME "RtmTable"
#define MOM_TBL_NAME "MomTable"
#define HOM_TBL_NAME "HomTable"
#define DOM_TBL_NAME "DomTable"
#define DIM_TBL_NAME "DimTable"
#define AOM_TBL_NAME "AomTable"

#define RUNTIME_INFO_UPDATE_INTERVAL_S 5

#define MAX_RI_REC_NUM 50
#define MAX_RTD_REC_NUM 20160
#define MAX_RTM_REC_NUM 20160 
#define MAX_MOM_REC_NUM 8640
#define MAX_HOM_REC_NUM 8760
#define MAX_DOM_REC_NUM 365 
#define MAX_DIM_REC_NUM 1000
#define MAX_AOM_REC_NUM 1000

/* Database Table Type */
typedef enum {
	RKRTDTABLE = 0,
	RKRTMTABLE,
	RKMOMTABLE,
	RKHOMTABLE,
	RKDOMTABLE,
} DBTABLE_T;

/* front-end acquisition data type */

typedef struct rkQueryCond{
	DBTABLE_T tabType; /* Table type */
	time_t startTm; /* Start time */
	time_t stopTm; /* Stop time */
	char code[8];
} rkQueryCond_t;

int rkDsmInit(struct context *ctx);
void rkDsmRun(void *handle);
int rkDsmSaveMsg(MSG_TYPE_T type, char *msg);
int rkDsmQuery(struct rkQueryCond *cond, char *rst);

#endif /* _RK_DSM_H_ */
