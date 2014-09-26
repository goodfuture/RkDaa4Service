/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.0
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkcou.c
 * Created At		: 2013-09-23 14:29
 * Last Modified	: 2013-09-29 15:01
 * Description		: 
 * History			: 
*******************************************************************************/

#include "rkcou.h"
#include <string.h>
#include <stdio.h>

extern struct context ctx;

int rkCouInitCTbl(struct codetbl *ctbl)
{
	ctbl->m_pCode[0] = "01";
	ctbl->m_pCode[1] = "02";
	ctbl->m_pCode[2] = "03";
	ctbl->m_pCode[3] = "B01";
	ctbl->m_pCode[4] = "B02";

	return 0;
}

int rkCouGetCTblIndex(const char *scode)
{
	int index;
	char *dcode;

	for (index = 0; index < MAX_CODE_TBL_NUM; index++) {
		dcode = ctx.m_tCodeTable.m_pCode[index];
		if (scode && dcode && !strcmp(scode, dcode)) {
			return index;
		}
	}

	return -1; 
}

int rkCouGetCTblVal(const char *code, float *val)
{
	int i;
	for (i = 0; i < MAX_CODE_TBL_NUM; i++) {
		if (code && !strcmp(code, ctx.m_tCodeTable.m_pCode[i])) {
			*val = ctx.m_tCodeTable.m_fValue[i];
			return 0;
		}   
	}   

	return -1;
}

int rkCouUpdateCTbl(const char *code, float val)
{
	int index;

	index = rkCouGetCTblIndex(code);

	if (index >= 0) {
		ctx.m_tCodeTable.m_fValue[index] = val;
		return 0;
	}

	return -1;
}

int rkCouCalcStat(const char *code, struct statinfo *stat, float val)
{
	int rc;
	float var;
	double rst;

	rc = rkCouUpdateCTbl(code, val);
	if (rc == -1) {
		return -1;
	}

	if (!strcmp(code, "01") || !strcmp(code, "02") || !strcmp(code, "03")) {
		/* mg/m3 --> kg/s */
		rc = rkCouGetCTblVal("B02", &var);
		var = !rc ? var : 0;
		rst = val * var / 1000000.0 / 3600.0;
	} else if (!strcmp(code, "B01")) {
		/* L/s --> m3/s */
		rst = val / 1000.0;
	} else if (!strcmp(code, "B02")) {
		/* m3/h --> m3/s */
		rst = val / 3600.0;
	}

	stat->cou[MNT_DATA_STAT_OFFSET] += rst;
	stat->cou[HOU_DATA_STAT_OFFSET] += rst;
	stat->cou[DAY_DATA_STAT_OFFSET] += rst;

	return 0;
}
