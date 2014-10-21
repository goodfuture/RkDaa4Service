/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.0
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkhjt.c
 * Created At		: 2013-09-23 14:29
 * Last Modified	: 2013-09-23 14:29
 * Description		: 
 * History			: 
*******************************************************************************/

#include "rkhjt.h"
#include "rkcou.h"
#include "rkxml.h"
#include <time.h>

/* g_ctx Struct */
static struct context *g_ctx;

const struct hjtCnMap hjtCnTbl[] = {
	/* Initialize Command */
	{ rkHjtExeCmd1000, 1000, 0 },
	{ rkHjtExeCmd1001, 1001, 0 },
	/* Parameter Command */
	{ rkHjtGenMsg1011, 1011, 1 },
	{ rkHjtExeCmd1012, 1012, 0 },
	{ rkHjtGenMsg1021, 1021, 1 },
	{ rkHjtExeCmd1022, 1022, 0 },
	{ rkHjtGenMsg1031, 1031, 1 },
	{ rkHjtExeCmd1032, 1032, 0 },
	{ rkHjtGenMsg1041, 1041, 1 },
	{ rkHjtExeCmd1042, 1042, 0 },
	{ rkHjtGenMsg1061, 1061, 1 },
	{ rkHjtExeCmd1062, 1062, 0 },
	{ rkHjtExeCmd1072, 1072, 0 },
	{ rkHjtGenMsg1091, 1091, 1 },
	{ rkHjtExeCmd1092, 1092, 0 },
	/* Data Command */
	{ rkHjtGenMsg2011, 2011, 1 },
	{ rkHjtExeCmd2012, 2012, 0 },
	{ rkHjtGenMsg2021, 2021, 1 },
	{ rkHjtExeCmd2022, 2022, 0 },
	{ rkHjtGenMsg2031, 2031, 1 },
	{ rkHjtGenMsg2041, 2041, 1 },
	{ rkHjtGenMsg2051, 2051, 1 },
	{ rkHjtGenMsg2061, 2061, 1 },
	{ rkHjtGenMsg2071, 2071, 1 },
	{ rkHjtGenMsg2072, 2072, 1 },
	{ rkHjtGenMsg2081, 2081, 1 },
	/* Control Command */
	{ rkHjtExeCmd3011, 3011, 0 },
	{ rkHjtExeCmd3012, 3012, 0 },
	{ rkHjtExeCmd3013, 3013, 0 },
	{ rkHjtExeCmd3014, 3014, 0 },
	/* Interactive Command */
	{ rkHjtGenMsg9011, 9011, 1 },
	{ rkHjtGenMsg9012, 9012, 1 },
	{ rkHjtGenMsg9013, 9013, 1 },
	{ rkHjtGenMsg9014, 9014, 1 },
	/* User Command */
	{ rkHjtGenMsg3051, 3051, 1 },
	{ NULL, 0, 0 },
};

static char *rkHjtFltToStr(float val)
{
	char *ptr;
	static char buf[256];

	sprintf(buf, "%.6f", val);
	ptr = buf + strlen(buf) - 1;
	for (; *ptr == '0'; ptr--) {
		*ptr = '\0';
	}

	if (*ptr == '.') {
		*ptr = '\0';
	}

	return buf;
}

int rkHjtInit(struct context *handle)
{
	g_ctx = (struct context *)handle;
	return !g_ctx ? -1 : 0;
}

int rkHjtParseMsg(char *msg, struct hjtPkt *pkt)
{
	char *ptr, temp[32];

	bzero(pkt, sizeof(struct hjtPkt));

	ptr = strstr(msg, "MN=");
	if (!ptr) {
		return HJTERRMSG;
	} else {
		strncpy(pkt->mn, ptr + 3, 14);
		if (strncmp(pkt->mn, g_ctx->m_tSystemParam.mn, 14) != 0) {
			return HJTERRMN;
		}
	}

	ptr = strstr(msg, "PW=");
	if (!ptr) {
		return HJTERRMSG;
	} else {
		strncpy(pkt->pw, ptr + 3, 6);
		if (strncmp(pkt->pw, g_ctx->m_tSystemParam.pw, 6) != 0) {
			return HJTERRPW;
		}
	}

	ptr = strstr(msg, "QN=");
	if (!ptr) {
		return HJTERRMSG;
	} else {
		strncpy(pkt->qn, ptr + 3, 17);
	}

	ptr = strstr(msg, "CN=");
	if (!ptr) {
		return HJTERRMSG;
	} else {
		bzero(temp, sizeof(temp));
		strncpy(temp, ptr + 3, 4);
		pkt->cn = atoi(temp);
	}

	ptr = strstr(msg, "Flag=");
	if (!ptr) {
		return HJTERRMSG;
	} else {
		memcpy(&pkt->flag, ptr + 5, 1);
	}

	ptr = strstr(msg, "CP=&&");
	if (!ptr) {
		return HJTERRMSG;
	} else {
		ptr += 5;
		char *p = strstr(ptr, "&&");
		if (!p) {
			return HJTERRMSG;
		}
		strncpy(pkt->cp, ptr, p - ptr);
	}

	return 0;
}

/* 
 * Function : Build Message Header 
 */
void rkHjtFillMsgHdr(uint16_t cn, char *buf)
{
	char st[3];

	if (cn == 9011 || cn == 9012 || cn == 9013) {
		sprintf(st, "91");
	} else {
		sprintf(st, "%d", g_ctx->m_tSystemParam.st);
	}

	if(!strlen(g_ctx->m_tSystemParam.sim)) {
		sprintf(buf,"##0000ST=%s;CN=%04d;PW=%s;MN=%s;", st, cn, g_ctx->m_tSystemParam.pw, g_ctx->m_tSystemParam.mn);
	} else {
#if 0
		sprintf(buf,"%s09##0000ST=%s;CN=%04d;PW=%s;MN=%s;", g_ctx->m_tSystemParam.sim, st, cn, g_ctx->m_tSystemParam.pw, g_ctx->m_tSystemParam.mn);
#else
		sprintf(buf,"%s##0000ST=%s;CN=%04d;PW=%s;MN=%s;", g_ctx->m_tSystemParam.sim, st, cn, g_ctx->m_tSystemParam.pw, g_ctx->m_tSystemParam.mn);
#endif
	}
}

/* 
 * Function : Build Message End
 */
void rkHjtFillMsgEnd(char *msg)
{
	int crc, pktlen, hdrlen;
	char tmp[8];

#if 1
	hdrlen = strlen(g_ctx->m_tSystemParam.sim);
	hdrlen = !hdrlen ? HJT_MSG_HDR_LEN : HJT_MSG_HDR_LEN + hdrlen;
	pktlen = strlen(msg + hdrlen);
	sprintf(tmp, "%04d", pktlen);
	memcpy(msg + hdrlen - 4, tmp, 4);
	crc = rkCrc16(msg + HJT_MSG_HDR_LEN, pktlen);
	sprintf(msg + hdrlen + pktlen, "%04X\r\n", crc);
#else
	hdrlen = strlen(g_ctx->m_tSystemParam.sim);
	hdrlen = !hdrlen ? HJT_MSG_HDR_LEN : HJT_MSG_HDR_LEN + hdrlen + 2;
	pktlen = strlen(msg + hdrlen);
	sprintf(tmp, "%04d", pktlen);
	memcpy(msg + hdrlen - 4, tmp, 4);
	crc = rkCrc16New(msg + HJT_MSG_HDR_LEN, pktlen);
	sprintf(msg + hdrlen + pktlen, "%04X\r\n", crc);
#endif
}

/* 
 * CN = 1000
 * Function : set overtime and retry upload times 
 */
int rkHjtExeCmd1000(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 1001
 * Function : set time of alarm 
 */
int rkHjtExeCmd1001(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/* 
 * CN = 1011
 * Function : upload local time to server
 */
int rkHjtGenMsg1011(struct hjtPkt *pkt, char *msg, void *arg)
{
	time_t ctm;
	struct tm tms;

	time(&ctm);
	localtime_r(&ctm, &tms);

	rkHjtFillMsgHdr(1011, msg);
	sprintf(msg+ strlen(msg), "CP=&&QN=%s;SystemTime=%04d%02d%02d%02d%02d%02d&&", pkt->qn, tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday, tms.tm_hour, tms.tm_min, tms.tm_sec);
	rkHjtFillMsgEnd(msg);

	return 0;
}

/*
 * CN = 1012
 * Function : set local time
 */
int rkHjtExeCmd1012(struct hjtPkt *pkt, char *msg, void *arg)
{
	struct tm stm;
	time_t ctm;
	char tmp[8], *pname = "SystemTime=";

	char *ptr = strstr(pkt->cp, pname);
	if (!ptr || strlen(ptr) < 14) {
		return 0;
	}

	ptr += strlen(pname);
	strncpy(tmp, ptr, 4);
	stm.tm_year = atoi(tmp) - 1900;
	bzero(tmp, sizeof(tmp));
	ptr += 4;

	strncpy(tmp, ptr, 2);
	stm.tm_mon = atoi(tmp) - 1;
	bzero(tmp, sizeof(tmp));
	ptr += 2;

	strncpy(tmp, ptr, 2);
	stm.tm_mday= atoi(tmp);
	bzero(tmp, sizeof(tmp));
	ptr += 2;

	strncpy(tmp, ptr, 2);
	stm.tm_hour = atoi(tmp);
	bzero(tmp, sizeof(tmp));
	ptr += 2;

	strncpy(tmp, ptr, 2);
	stm.tm_min = atoi(tmp);
	bzero(tmp, sizeof(tmp));
	ptr += 2;

	strncpy(tmp, ptr, 2);
	stm.tm_sec = atoi(tmp);
	bzero(tmp, sizeof(tmp));
	ptr += 2;

	ctm = mktime(&stm);

	return stime(&ctm);
}

/*
 * CN = 1021
 * Function : upload alarm limit values
 */
int rkHjtGenMsg1021(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 1022
 * Function : set alarm limit values
 */
int rkHjtExeCmd1022(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 1031
 * Function : upload upper computer address
 */
int rkHjtGenMsg1031(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 1032
 * Function : set upper computer address
 */
int rkHjtExeCmd1032(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 1041
 * Function : upload 'data upload interval'
 */
int rkHjtGenMsg1041(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 1042
 * Function : set 'data upload interval'
 */
int rkHjtExeCmd1042(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 1061
 * Function : upload read-time data acquisition interval
 */
int rkHjtGenMsg1061(struct hjtPkt *pkt, char *msg, void *arg)
{
	rkHjtFillMsgHdr(1061, msg);
	sprintf(msg + strlen(msg), "CP=&&QN=%s;RtdInterval=%d&&", pkt->qn, g_ctx->m_tSystemParam.dui);
	rkHjtFillMsgEnd(msg);

	return 0;
}

/*
 * CN = 1062
 * Function : set Rtd interval
 */
int rkHjtExeCmd1062(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 1072
 * Function : set system password
 */
int rkHjtExeCmd1072(struct hjtPkt *pkt, char *msg, void  *arg)
{
	mxml_node_t *root;

	char *ptr = strstr(pkt->cp, "PW=");
	if (!ptr) {
		return -1;
	}

	root = rkXmlLoadFile(SYS_CFG_FILE);
	if (!root) {
		return -1;
	}

	rkXmlSetParamVal(root, "PW", ptr + 3);
	rkXmlSaveFile(SYS_CFG_FILE, root);
	rkXmlFree(root);

	return 0;
}

/*
 * CN = 1091
 * Function : upload SIM card number
 */
int rkHjtGenMsg1091(struct hjtPkt *pkt, char *msg, void *arg)
{
	rkHjtFillMsgHdr(1091, msg);
	sprintf(msg + strlen(msg), "CP=&&QN=%s;SIM=%s&&", pkt->qn, g_ctx->m_tSystemParam.sim);
	rkHjtFillMsgEnd(msg);

	return 0;
}

/*
 * CN = 1092
 * Function : set SIM card number
 */
int rkHjtExeCmd1092(struct hjtPkt *pkt, char *msg, void  *arg)
{
	char *ptr = strstr(pkt->cp, "SIM=");
	if (!ptr) {
		return -1;
	}

	int ret = sscanf(ptr, "SIM=%s", g_ctx->m_tSystemParam.sim);

	return ret == 1 ? 0 : -1;
}

/*
 * CN = 2011
 * Function : upload real-time message 
 */
int rkHjtGenMsg2011(struct hjtPkt *pkt, char *msg, void *arg)
{
	uint16_t i, j;
	time_t ctm;
	struct tm *tms;
	static uint16_t ai_pos = 0, ei_pos = 0;
	struct aiparam *ai[2];
	struct edev *ei[2];

	time(&ctm);
	tms = localtime(&ctm);

	rkHjtFillMsgHdr(2011, msg);
	sprintf(msg + strlen(msg), "CP=&&DataTime=%04d%02d%02d%02d%02d%02d", 
			tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday, tms->tm_hour, tms->tm_min, tms->tm_sec);

	if (ei_pos > 0) {
		goto EI_MSG_2011;
	}

	for (i = ai_pos; i < AI_NUM; i++) {
		ai[0] = &g_ctx->m_tAnalogParam.m_tChannelParam[i];
		if (!ai[0]->inuse || !strlen(ai[0]->code) || ai[0]->isconv) {
			continue;
		}

		sprintf(msg + strlen(msg), ";%s-Rtd=%s", ai[0]->code, rkHjtFltToStr(ai[0]->vals[RTD_CONV_VAL_OFFSET]));

		for (j = 0; j < AI_NUM; j++) {
			ai[1] = &g_ctx->m_tAnalogParam.m_tChannelParam[j];
			if (!ai[1]->inuse || !ai[1]->isconv || strcmp(ai[1]->code, ai[0]->code)) {
				continue;
			}
			sprintf(msg + strlen(msg), ",%s-ZsRtd=%s", ai[1]->code, rkHjtFltToStr(ai[1]->vals[RTD_CONV_VAL_OFFSET]));
		}

		sprintf(msg + strlen(msg), ",%s-Flag=%c", ai[0]->code, ai[0]->flag);

		if (strlen(msg) >= HJT_MSG_SAFE_LEN) {
			ai_pos = i + 1;
			strcat(msg,"&&");
			rkHjtFillMsgEnd(msg);
			return 1;
		}
	}
	ai_pos = 0;

EI_MSG_2011:
	for (i = ei_pos; i < EI_NUM; i++) {
		ei[0] = &g_ctx->m_tUartParam.m_tChannelParam[i];
		if (!ei[0]->inuse || ei[0]->isconv || !strlen(ei[0]->code)) {
			continue;
		}

		sprintf(msg + strlen(msg), ";%s-Rtd=%s", ei[0]->code, rkHjtFltToStr(ei[0]->vals[RTD_CONV_VAL_OFFSET]));
		for (j = 0; j < EI_NUM; j++) {
			ei[1] = &g_ctx->m_tUartParam.m_tChannelParam[j];
			if (!ei[1]->inuse || !ei[1]->isconv || strcmp(ei[0]->code, ei[1]->code)) {
				continue;
			}
			sprintf(msg + strlen(msg), ",%s-ZsRtd=%s", ei[1]->code, rkHjtFltToStr(ei[1]->vals[RTD_CONV_VAL_OFFSET]));
		}

		sprintf(msg + strlen(msg), ",%s-Flag=%c", ei[0]->code, ei[0]->flag);

		if (strlen(msg) >= HJT_MSG_SAFE_LEN) {
			ei_pos = i + 1;
			strcat(msg,"&&");
			rkHjtFillMsgEnd(msg);
			return 1;
		}
	}
	ei_pos = 0;

	strcat(msg,"&&");
	rkHjtFillMsgEnd(msg);

	return 0;
}

/*
 * CN = 2012
 * Function : upload real-time message 
 */
int rkHjtExeCmd2012(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 2021
 * Function : upload device-state message 
 */
int rkHjtGenMsg2021(struct hjtPkt *pkt, char *msg, void *arg)
{
	uint16_t i;
	time_t ctm;
	struct tm *tms;

	time(&ctm);
	tms = localtime(&ctm);

	rkHjtFillMsgHdr(2021, msg);
	sprintf(msg + strlen(msg), "CP=&&DataTime=%04d%02d%02d%02d%02d%02d", 
			tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday, tms->tm_hour, tms->tm_min, tms->tm_sec);

	for (i = 0; i < DI_NUM; i++) {
		struct dioparam *di = &g_ctx->m_tDioParam.m_tDiParam[i];
		if (!di->inuse || !strlen(di->code)) {
			continue;
		}
		sprintf(msg + strlen(msg), ";%s=%d", di->code, di->val);
	}

	strcat(msg,"&&");
	rkHjtFillMsgEnd(msg);

	return 0;
}

/*
 * CN = 2022
 * Function : stop upload device-state message
 */
int rkHjtExeCmd2022(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 2031
 * Function : upload daily message 
 */
int rkHjtGenMsg2031(struct hjtPkt *pkt, char *msg, void *arg)
{
	uint16_t i;
	time_t ctm;
	struct tm *tms;
	static uint16_t ai_pos = 0, ei_pos = 0;

	time(&ctm);
	tms = localtime(&ctm);

	rkHjtFillMsgHdr(2031, msg);
	sprintf(msg + strlen(msg), "CP=&&DataTime=%04d%02d%02d000000", 
			tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday);

	if (ei_pos > 0) {
		goto EI_MSG_2031;
	}

	for (i = ai_pos; i < AI_NUM; i++) {
		struct aiparam *ai = &g_ctx->m_tAnalogParam.m_tChannelParam[i];
		if (!ai->inuse || !strlen(ai->code)) {
			continue;
		}

		if (!ai->isconv) {
			sprintf(msg + strlen(msg), ";%s-Min=%s,%s-Avg=%s,%s-Max=%s", 
					ai->code, rkHjtFltToStr(ai->stat.min[DAY_DATA_STAT_OFFSET]), 
					ai->code, rkHjtFltToStr(ai->stat.avg[DAY_DATA_STAT_OFFSET]), 
					ai->code, rkHjtFltToStr(ai->stat.max[DAY_DATA_STAT_OFFSET]));
		} else {
			sprintf(msg + strlen(msg), ";%s-ZsMin=%s,%s-ZsAvg=%s,%s-ZsMax=%s", 
					ai->code, rkHjtFltToStr(ai->stat.min[DAY_DATA_STAT_OFFSET]), 
					ai->code, rkHjtFltToStr(ai->stat.avg[DAY_DATA_STAT_OFFSET]),
					ai->code, rkHjtFltToStr(ai->stat.max[DAY_DATA_STAT_OFFSET]));
		}

		if (rkCouGetCTblIndex(ai->code) != -1) {
			sprintf(msg + strlen(msg), ",%s-Cou=%s", ai->code, rkHjtFltToStr(ai->stat.cou[DAY_DATA_STAT_OFFSET]));
		}

		if (strlen(msg) >= HJT_MSG_SAFE_LEN) {
			ai_pos = i + 1;
			strcat(msg,"&&");
			rkHjtFillMsgEnd(msg);
			return 1;
		}
	}
	ai_pos = 0;

EI_MSG_2031:
	for (i = ei_pos; i < EI_NUM; i++) {
		struct edev *ei = &g_ctx->m_tUartParam.m_tChannelParam[i];
		if (!ei->inuse || !strlen(ei->code)) {
			continue;
		}

		if (!ei->isconv) {
			sprintf(msg + strlen(msg), ";%s-Min=%s,%s-Avg=%s,%s-Max=%s", 
					ei->code, rkHjtFltToStr(ei->stat.min[DAY_DATA_STAT_OFFSET]), 
					ei->code, rkHjtFltToStr(ei->stat.avg[DAY_DATA_STAT_OFFSET]), 
					ei->code, rkHjtFltToStr(ei->stat.max[DAY_DATA_STAT_OFFSET]));
		} else {
			sprintf(msg + strlen(msg), ";%s-ZsMin=%s,%s-ZsAvg=%s,%s-ZsMax=%s", 
					ei->code, rkHjtFltToStr(ei->stat.min[DAY_DATA_STAT_OFFSET]), 
					ei->code, rkHjtFltToStr(ei->stat.avg[DAY_DATA_STAT_OFFSET]), 
					ei->code, rkHjtFltToStr(ei->stat.max[DAY_DATA_STAT_OFFSET]));
		}

		if (rkCouGetCTblIndex(ei->code) != -1) {
			sprintf(msg + strlen(msg), ",%s-Cou=%s", ei->code, rkHjtFltToStr(ei->stat.cou[DAY_DATA_STAT_OFFSET]));
		}

		if (strlen(msg) >= HJT_MSG_SAFE_LEN) {
			ei_pos = i + 1;
			strcat(msg,"&&");
			rkHjtFillMsgEnd(msg);
			return 1;
		}
	}
	ei_pos = 0;

	strcat(msg,"&&");
	rkHjtFillMsgEnd(msg);

	return 0;
}

/*
 * CN = 2041
 * Function : upload device runtime daily history message
 */
int rkHjtGenMsg2041(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 2051
 * Function : upload minutes message 
 */
int rkHjtGenMsg2051(struct hjtPkt *pkt, char *msg, void *arg)
{
	uint16_t i;
	time_t ctm;
	struct tm *tms;
	static uint16_t ai_pos = 0, ei_pos = 0;

	time(&ctm);
	tms = localtime(&ctm);

	rkHjtFillMsgHdr(2051, msg);
	sprintf(msg + strlen(msg), "CP=&&DataTime=%04d%02d%02d%02d%02d00", 
			tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday, tms->tm_hour, tms->tm_min);

	if (ei_pos > 0) {
		goto EI_MSG_2051;
	}

	for (i = ai_pos; i < AI_NUM; i++) {
		struct aiparam *ai = &g_ctx->m_tAnalogParam.m_tChannelParam[i];
		if (!ai->inuse || !strlen(ai->code)) {
			continue;
		}

		if (!ai->isconv) {
			sprintf(msg + strlen(msg), ";%s-Min=%s,%s-Avg=%s,%s-Max=%s", 
					ai->code, rkHjtFltToStr(ai->stat.min[MNT_DATA_STAT_OFFSET]), 
					ai->code, rkHjtFltToStr(ai->stat.avg[MNT_DATA_STAT_OFFSET]), 
					ai->code, rkHjtFltToStr(ai->stat.max[MNT_DATA_STAT_OFFSET]));
		} else {
			sprintf(msg + strlen(msg), ";%s-ZsMin=%s,%s-ZsAvg=%s,%s-ZsMax=%s", 
					ai->code, rkHjtFltToStr(ai->stat.min[MNT_DATA_STAT_OFFSET]), 
					ai->code, rkHjtFltToStr(ai->stat.avg[MNT_DATA_STAT_OFFSET]),
					ai->code, rkHjtFltToStr(ai->stat.max[MNT_DATA_STAT_OFFSET]));
		}

		if (rkCouGetCTblIndex(ai->code) != -1) {
			sprintf(msg + strlen(msg), ",%s-Cou=%s", ai->code, rkHjtFltToStr(ai->stat.cou[MNT_DATA_STAT_OFFSET]));
		}

		if (strlen(msg) >= HJT_MSG_SAFE_LEN) {
			ai_pos = i + 1;
			strcat(msg, "&&");
			rkHjtFillMsgEnd(msg);
			return 1;
		}
	}
	ai_pos = 0;

EI_MSG_2051:
	for (i = ei_pos; i < EI_NUM; i++) {
		struct edev *ei = &g_ctx->m_tUartParam.m_tChannelParam[i];
		if (!ei->inuse || !strlen(ei->code)) {
			continue;
		}

		if (!ei->isconv) {
			sprintf(msg + strlen(msg), ";%s-Min=%s,%s-Avg=%s,%s-Max=%s", 
					ei->code, rkHjtFltToStr(ei->stat.min[MNT_DATA_STAT_OFFSET]),
					ei->code, rkHjtFltToStr(ei->stat.avg[MNT_DATA_STAT_OFFSET]),
					ei->code, rkHjtFltToStr(ei->stat.max[MNT_DATA_STAT_OFFSET]));
		} else {
			sprintf(msg + strlen(msg), ";%s-ZsMin=%s,%s-ZsAvg=%s,%s-ZsMax=%s", 
					ei->code, rkHjtFltToStr(ei->stat.min[MNT_DATA_STAT_OFFSET]),
					ei->code, rkHjtFltToStr(ei->stat.avg[MNT_DATA_STAT_OFFSET]),
					ei->code, rkHjtFltToStr(ei->stat.max[MNT_DATA_STAT_OFFSET]));
		}

		if (rkCouGetCTblIndex(ei->code) != -1) {
			sprintf(msg + strlen(msg), ",%s-Cou=%s", ei->code, rkHjtFltToStr(ei->stat.cou[MNT_DATA_STAT_OFFSET]));
		}

		if (strlen(msg) >= HJT_MSG_SAFE_LEN) {
			ei_pos = i + 1;
			strcat(msg,"&&");
			rkHjtFillMsgEnd(msg);
			return 1;
		}
	}
	ei_pos = 0;

	strcat(msg,"&&");
	rkHjtFillMsgEnd(msg);

	return 0;
}

/*
 * CN = 2061
 * Function : upload hour message 
 */
int rkHjtGenMsg2061(struct hjtPkt *pkt, char *msg, void *arg)
{
	uint16_t i;
	time_t ctm;
	struct tm *tms;
	static uint16_t ai_pos = 0, ei_pos = 0;

	time(&ctm);
	tms = localtime(&ctm);

	rkHjtFillMsgHdr(2061, msg);
	sprintf(msg + strlen(msg), "CP=&&DataTime=%04d%02d%02d%02d0000", 
			tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday, tms->tm_hour);

	if (ei_pos > 0) {
		goto EI_MSG_2061;
	}

	for (i = ai_pos; i < AI_NUM; i++) {
		struct aiparam *ai = &g_ctx->m_tAnalogParam.m_tChannelParam[i];
		if (!ai->inuse || !strlen(ai->code)) {
			continue;
		}

		if (!ai->isconv) {
			sprintf(msg + strlen(msg), ";%s-Min=%s,%s-Avg=%s,%s-Max=%s", 
					ai->code, rkHjtFltToStr(ai->stat.min[HOU_DATA_STAT_OFFSET]),
					ai->code, rkHjtFltToStr(ai->stat.avg[HOU_DATA_STAT_OFFSET]),
					ai->code, rkHjtFltToStr(ai->stat.max[HOU_DATA_STAT_OFFSET]));
		} else { 
			sprintf(msg + strlen(msg), ";%s-ZsMin=%s,%s-ZsAvg=%s,%s-ZsMax=%s", 
					ai->code, rkHjtFltToStr(ai->stat.min[HOU_DATA_STAT_OFFSET]), 
					ai->code, rkHjtFltToStr(ai->stat.avg[HOU_DATA_STAT_OFFSET]),
					ai->code, rkHjtFltToStr(ai->stat.max[HOU_DATA_STAT_OFFSET]));
		}

		if (rkCouGetCTblIndex(ai->code) != -1) {
			sprintf(msg + strlen(msg), ",%s-Cou=%s", ai->code, rkHjtFltToStr(ai->stat.cou[HOU_DATA_STAT_OFFSET]));
		}

		if (strlen(msg) >= HJT_MSG_SAFE_LEN) {
			ai_pos = i + 1;
			strcat(msg,"&&");
			rkHjtFillMsgEnd(msg);
			return 1;
		}
	}
	ai_pos = 0;

EI_MSG_2061:
	for (i = ei_pos; i < EI_NUM; i++) {
		struct edev *ei = &g_ctx->m_tUartParam.m_tChannelParam[i];
		if (!ei->inuse || !strlen(ei->code)) {
			continue;
		}

		if (!ei->isconv) {
			sprintf(msg + strlen(msg), ";%s-Min=%s,%s-Avg=%s,%s-Max=%s", 
					ei->code, rkHjtFltToStr(ei->stat.min[HOU_DATA_STAT_OFFSET]),
					ei->code, rkHjtFltToStr(ei->stat.avg[HOU_DATA_STAT_OFFSET]),
					ei->code, rkHjtFltToStr(ei->stat.max[HOU_DATA_STAT_OFFSET]));
		} else {
			sprintf(msg + strlen(msg), ";%s-ZsMin=%s,%s-ZsAvg=%s,%s-ZsMax=%s", 
					ei->code, rkHjtFltToStr(ei->stat.min[HOU_DATA_STAT_OFFSET]),
					ei->code, rkHjtFltToStr(ei->stat.avg[HOU_DATA_STAT_OFFSET]),
					ei->code, rkHjtFltToStr(ei->stat.max[HOU_DATA_STAT_OFFSET]));
		}

		if (rkCouGetCTblIndex(ei->code) != -1) {
			sprintf(msg + strlen(msg), ",%s-Cou=%s", ei->code, rkHjtFltToStr(ei->stat.cou[HOU_DATA_STAT_OFFSET]));
		}

		if (strlen(msg) >= HJT_MSG_SAFE_LEN) {
			ei_pos = i + 1;
			strcat(msg,"&&");
			rkHjtFillMsgEnd(msg);
			return 1;
		}
	}
	ei_pos = 0;

	strcat(msg,"&&");
	rkHjtFillMsgEnd(msg);

	return 0;
}

/*
 * CN = 2071
 * Function : upload alarm message 
 */
int rkHjtGenMsg2071(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 2072
 * Function : upload alarm event 
 * Parameter : 'arg' - alarm type, must not be NULL
 */
int rkHjtGenMsg2072(struct hjtPkt *pkt, char *msg, void *arg)
{
	uint16_t i;
	time_t ctm;
	struct tm *tms;

	time(&ctm);
	tms = localtime(&ctm);

	rkHjtFillMsgHdr(2072, msg);
	sprintf(msg + strlen(msg), "CP=&&DataTime=%04d%02d%02d%02d0000", 
			tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday, tms->tm_hour);

	for (i = 0; i < AI_NUM; i++) {
		struct aiparam *ai = &g_ctx->m_tAnalogParam.m_tChannelParam[i];
		if (ai->inuse && strlen(ai->code) && ai->alarm) {
			sprintf(msg + strlen(msg), ";%s-Ala=%.6f;AlarmType=%d", ai->code, ai->vals[RTD_CONV_VAL_OFFSET], *(int *)arg);
		}
	}

	strcat(msg,"&&");
	rkHjtFillMsgEnd(msg);

	return 0;
}

/*
 * CN = 2081
 * Function : upload history message 
 */
int rkHjtGenMsg2081(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 3011
 * Function : Zero & Full Calibrate
 */
int rkHjtExeCmd3011(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 3012
 * Function : Sample Immediately
 */
int rkHjtExeCmd3012(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 3013
 * Function : Operation Devices
 */
int rkHjtExeCmd3013(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 3014
 * Function : Set Sample Cycle
 */
int rkHjtExeCmd3014(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 9011
 */
int rkHjtGenMsg9011(struct hjtPkt *pkt, char *msg, void *arg)
{
	rkHjtFillMsgHdr(9011, msg);
	sprintf(msg + strlen(msg), "Flag=0;CP=&&QN=%s;QnRtn=%d&&", pkt->qn, *(int *)arg);
	rkHjtFillMsgEnd(msg);

	return 0;
}

/*
 * CN = 9012
 */
int rkHjtGenMsg9012(struct hjtPkt *pkt, char *msg, void *arg)
{
	rkHjtFillMsgHdr(9012, msg);
	sprintf(msg + strlen(msg), "Flag=0;CP=&&QN=%s;ExeRtn=%d&&", pkt->qn, *(int *)arg);
	rkHjtFillMsgEnd(msg);

	return 0;
}

/*
 * CN = 9013
 */
int rkHjtGenMsg9013(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 9014
 */
int rkHjtGenMsg9014(struct hjtPkt *pkt, char *msg, void *arg)
{
	return 0;
}

/*
 * CN = 3051
 */
int rkHjtGenMsg3051(struct hjtPkt *pkt, char *msg, void *arg)
{
	int i;
	time_t ctm;
	struct tm *tms;
	struct dioparam *dip;

	time(&ctm);
	tms = localtime(&ctm);

	rkHjtFillMsgHdr(3051, msg);
	sprintf(msg + strlen(msg), "CP=&&DataTime=%04d%02d%02d%02d%02d%02d", 
			tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday, tms->tm_hour, tms->tm_min, tms->tm_sec);

	for (i = 0; i < DI_NUM; i++) {
		dip = &g_ctx->m_tDioParam.m_tDiParam[i];
		if (!dip->inuse || !strlen(dip->code)) {
			continue;
		}
		sprintf(msg + strlen(msg), ";%s=%d", dip->code, dip->val);
	}
	strcat(msg + strlen(msg), "&&");
	rkHjtFillMsgEnd(msg);

	return 0;
}
