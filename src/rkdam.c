/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.0
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkdam.c
 * Created At		: 2013-09-23 08:35
 * Last Modified	: 2013-09-27 15:50
 * Description		: 
 * History			: 
*******************************************************************************/
#include "rkdam.h"
#include "rkfml.h"
#include "rkxml.h"
#include "rkser.h"
#include "rkscan.h"
#include "rkcou.h"
#include "rkdebug.h"
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>

extern struct context ctx;
/* 
 * Function : Digital input and output initialize 
 */
static int rkDamDioInit(struct dio *dio)
{
	int i;

	for (i = 0; i < DI_NUM; i++) {
		pthread_mutex_init(&dio->m_tDiParam[i].lock, NULL);
	}

	for (i = 0; i < DO_NUM; i++) {
		pthread_mutex_init(&dio->m_tDoParam[i].lock, NULL);
	}

	return 0;
}

/* 
 * Function : Analog initialize 
 */
static int rkDamAiInit(struct analog *analog)
{
	int index;

	for (index = 0; index < AI_NUM; index++) {
		if (analog->m_tChannelParam[index].utv > analog->m_tChannelParam[index].ulv) {
			analog->m_tChannelParam[index].utv = analog->m_tChannelParam[index].ulv;
		}
		if (analog->m_tChannelParam[index].ltv < analog->m_tChannelParam[index].llv) {
			analog->m_tChannelParam[index].ltv = analog->m_tChannelParam[index].llv;
		}
		analog->m_tCalibrateParam[index].loffset = 0x0002;
		analog->m_tCalibrateParam[index].hoffset = 0x587A;
		pthread_mutex_init(&analog->m_tChannelParam[index].lock, NULL);
	}
	rkXmlParseAiCalParam(analog);

	return 0;
}

/* 
 * Function : External Instrument initialize 
 */
static int rkDamEiInit(struct uart *uart)
{
	int i;
	struct com *com;
	struct edev *ei;

	for (i = 0; i < EI_NUM; i++) {
		ei = &uart->m_tChannelParam[i];
		ei->id = i;
		com = &uart->m_tComParam[ei->com];
		if (!ei->inuse) {
			continue;
		}
		ei->fd = com->fd <= 0 ? rkSerOpen(com) : com->fd;
		if (ei->fd <= 0) {
			return -1;
		}
		//printf("com[%d]->fd = %d\n", com->id, com->fd);
		pthread_mutex_init(&ei->mutex, NULL);
	}

	return 0;
}

int rkDamInit(struct context *ctx)
{
	int rc;

	rc = rkFmlSymTblInit();
	if (rc == -1) {
		return -1;
	}

	rc = rkDamDioInit(&ctx->m_tDioParam);
	if (rc == -1) {
		return -1;
	}

	rc = rkDamAiInit(&ctx->m_tAnalogParam);
	if (rc == -1) {
		return -1;
	}

	rc = rkDamEiInit(&ctx->m_tUartParam);
	if (rc == -1) {
		return -1;
	}

	rc = rkLibScan(ctx->m_pCmdLineArg[ARG_DYN_LIB_OFFSET]);
	if (rc == -1) {
		return -1;
	}

	return 0;
}

int rkDamAiDataSample(struct aiparam *ai)
{
	int fd, ret;
	char dev[64];
	uint16_t rawval;

	sprintf(dev, "%s%d", AI_DEV_NAME_PREFIX, ai->id);

	fd = open(dev, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s : open \'%s\' failed.\n", __func__, dev);
		return -1;
	}

	ret = read(fd, &rawval, sizeof(uint16_t));
	if (ret != sizeof(uint16_t)) {
		fprintf(stderr, "%s : read \'%s\' failed.\n", __func__, dev);
		close(fd);
		return -1;
	}
	close(fd);

	ai->vals[RTD_RAW_VAL_OFFSET] = (float)rawval;

	return 0;
}

int rkDamAiDataProc(struct aiparam *ai, struct aical *cal)
{
	int ret = 0;
	char tmp[64] = { 0 };
	float pit, rate;
	float rawval, fineval;

	rawval = ai->vals[RTD_RAW_VAL_OFFSET];

	/* Process Sample Data */
	if(ai->type == MA20) { /* 4-20mA */
		pit = (cal->hoffset - cal->loffset) / 5.0;
		rate = (ai->ulv - ai->llv) / (pit * 4.0);

		if(rawval < pit) {
			fineval = ai->llv;
		} else if(rawval > (float)cal->hoffset) {
			fineval = ai->ulv;
		} else {
			fineval = rate * (rawval - pit) + ai->llv;
		}
	} else if (ai->type == V5 || ai->type == V10) { /* 0-5v */
		rate = (ai->ulv - ai->llv) / (cal->hoffset - cal->loffset);
		fineval = rate * (rawval - cal->loffset) + ai->llv;
	}

	if (fineval > ai->ulv) {
		fineval = ai->ulv;
	} else if (fineval < ai->llv) {
		fineval = ai->llv;
	}

	/* Formula Calculate */
	if (ai->usefml) {
		rkFmlUpdateSymTbl(FML_VAR_PREFIX, fineval);
		if (rkFmlConvExpr(ai->fml, tmp, ai->code) == -1) {
			ai->alarm = ALARM_E;
			return -1;
		} 
		fineval = rkFmlEvaluateExpr(tmp);
		sprintf(tmp, "%s%s", FML_VAR_PREFIX, ai->code);
		rkFmlUpdateSymTbl(tmp, fineval);
	}

	ai->vals[RTD_CONV_VAL_OFFSET] = fineval;

	if (fineval < ai->ltv) {
		ai->alarm = ALARM_L;
	} else if (fineval > ai->utv) {
		ai->alarm = ALARM_H;
	} else {
		ai->alarm = ALARM_N;
	}

	rkDamCalcStat(ai->code, &ai->stat, ai->vals[RTD_CONV_VAL_OFFSET]);

	return ret;
}

int rkDamCalcStat(const char *code, struct statinfo *stat, float val)
{
	int i;

	for (i = 0; i < 4; i++) {
		if (!stat->cnt[i]) {
			stat->min[i] = val;
			stat->max[i] = val;
		} else {
			stat->min[i] = val < stat->min[i] ? val : stat->min[i];
			stat->max[i] = val > stat->max[i] ? val : stat->max[i];
		}
		stat->cnt[i]++;
		stat->sum[i] += val;
		stat->avg[i] = stat->sum[i] / stat->cnt[i];
	}

	return 0;
}

int rkDamClrStatData(struct context *ctx, STAT_DATA_TYPE type)
{
	int i;
	struct statinfo *stat;

	for (i = 0; i < AI_NUM; i++) {
		stat = &ctx->m_tAnalogParam.m_tChannelParam[i].stat;
		stat->cnt[type] = 0;
		stat->min[type] = 0;
		stat->max[type] = 0;
		stat->avg[type] = 0;
		stat->sum[type] = 0;
	}

	for (i = 0; i < EI_NUM; i++) {
		stat = &ctx->m_tUartParam.m_tChannelParam[i].stat;
		stat->cnt[type] = 0;
		stat->min[type] = 0;
		stat->max[type] = 0;
		stat->avg[type] = 0;
		stat->sum[type] = 0;
	}

	return 0;
}

int rkDamGetAiVal(struct analog *analog)
{
	int i, rc1, rc2;
	struct aiparam *ai;
	static time_t ltm = 0, ctm;

	for (i = 0; i < AI_NUM; i++) {
		ai = &analog->m_tChannelParam[i];
		if (ai->inuse) {
			pthread_mutex_lock(&ai->lock);
			rc1 = rkDamAiDataSample(ai);
			rc2 = rkDamAiDataProc(ai, &analog->m_tCalibrateParam[i]);
			if (rc1 == -1 || rc2 == -1) {
				ai->flag = 'D';
			} else {
				ai->flag = 'N';
			}
			pthread_mutex_unlock(&ai->lock);
		}
	}

	time(&ctm);
	if (ctm - ltm >= 1) {
		ltm = ctm;
		for (i = 0; i < AI_NUM; i++) {
			ai = &analog->m_tChannelParam[i];
			rkCouCalcStat(ai->code, &ai->stat, ai->vals[RTD_CONV_VAL_OFFSET]);
		}
	}

	return 0;
}

int rkDamPushAiInfo(struct analog *analog)
{
	int i;
	struct rtdInfo rtd;
	struct aiparam *ai;

	for (i = 0; i < AI_NUM; i++) {
		ai = &analog->m_tChannelParam[i];
		rtd.id = ai->id;
		rtd.inuse = ai->inuse;
		strncpy(rtd.code, ai->code, sizeof(rtd.code) - 1);
		rtd.vals[RTV_OFFSET] = ai->vals[RTD_CONV_VAL_OFFSET];
		rtd.vals[TAV_OFFSET] = ai->stat.avg[MNT_DATA_STAT_OFFSET];
		rtd.vals[HAV_OFFSET] = ai->stat.avg[HOU_DATA_STAT_OFFSET];
		rtd.vals[DAV_OFFSET] = ai->stat.avg[DAY_DATA_STAT_OFFSET];
		rtd.vals[MAV_OFFSET] = ai->stat.avg[MON_DATA_STAT_OFFSET];
		rtd.alm = ai->alarm;
		rkShmPushData(TYPE_AI, i, &rtd);
	}

	return 0;
}

int rkDamPushDiInfo(struct dioparam *dip)
{
	int i;
	struct dioInfo dioinfo;

	for (i = 0; i < DI_NUM; i++) {
		dioinfo.id = dip[i].id;
		dioinfo.inuse = dip[i].inuse;
		strncpy(dioinfo.code, dip[i].code, sizeof(dioinfo.code) - 1);
		dioinfo.val = dip[i].val;
		rkShmPushData(TYPE_DI, i, &dioinfo);
	}

	return 0;
}

int rkDamPushDoInfo(struct dioparam *dop)
{
	int i;
	struct dioInfo dioinfo;

	for (i = 0; i < DO_NUM; i++) {
		dioinfo.id = dop[i].id;
		dioinfo.inuse = dop[i].inuse;
		strncpy(dioinfo.code, dop[i].code, sizeof(dioinfo.code) - 1);
		dioinfo.val = dop[i].val;
		rkShmPushData(TYPE_DO, i, &dioinfo);
	}

	return 0;
}

int rkDamDiRead(uint16_t *val)
{
	uint16_t dival;

	int fd = open(DIO_DEV_INTERFACE, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s : open file \'%s\' failed : %s.\n", __func__, DIO_DEV_INTERFACE, strerror(errno));
		return -1;
	}

	int ret = read(fd, &dival, sizeof(uint16_t));
	if (ret != sizeof(uint16_t)) {
		fprintf(stderr, "%s : read file \'%s\' failed : %s.\n", __func__, DIO_DEV_INTERFACE, strerror(errno));
		close(fd);
		return -1;
	}
	close(fd);

	*val = ~dival;
	//*val = dival;
	//printf("di = 0x%04X\n", *val);

	return 0;
}

int rkDamGetDiVal(struct dio *dio)
{
	uint16_t di, tmp, i;

	if (rkDamDiRead(&di) == -1) {
		return -1;
	}
	//printf("di2 = 0x%04X\n", di);

	for (i = 0; i < DI_NUM; i++) {
		struct dioparam *dip = &dio->m_tDiParam[i];
		if (dip->inuse == 0) {
			continue;
		}

		if (di & (1 << (DI_NUM - i - 1))) {
			tmp = 1;
		} else {
			tmp = 0;
		}
		//tmp = di & (1 << i);
		//printf("tmp = %d\n", tmp);
		if (tmp != dip->val) {
			dio->m_uDiChangedFlag = 1;
		}

		pthread_mutex_lock(&dip->lock);
		dip->val = tmp;
		pthread_mutex_unlock(&dip->lock);
	}

	return 0;
}

int rkDamSetDoVal(struct dioparam *dop)
{
	int fd, i;
	uint8_t val;

	fd = open(DIO_DEV_INTERFACE, O_WRONLY);
	if(fd<0) {
		fprintf(stderr, "%s : open file \'%s\' failed : %s.\n", __func__, DIO_DEV_INTERFACE, strerror(errno));
		return -1;
	}

	memset(&val, 0, sizeof(val));
	for (i = 0; i < DO_NUM; i++) {
		if (dop[i].inuse && dop[i].val) {
			val |= 1 << i;
		}
	}
	write(fd, &val, sizeof(uint8_t));
	close(fd);

	return 0;
}

void rkDamRun(void *handle)
{
	int rc1, rc2;
	pthread_t threadIo, threadEi;

	rc1 = pthread_create(&threadIo, NULL, (void *)rkDamIoThread, handle);
	if (rc1 != 0) {
		fprintf(stderr, "Can't create IO thread : %s.\n", strerror(rc1));
		pthread_exit(&rc1);
	}


	rc2 = pthread_create(&threadEi, NULL, (void *)rkDamEiThread, handle);
	if (rc2 != 0) {
		fprintf(stderr, "Can't create EI thread : %s.\n", strerror(rc2));
		pthread_exit(&rc2);
	}

	rc1 = pthread_join(threadIo, NULL);
	if (rc1 != 0) {
		fprintf(stderr, "Thread 'threadIo' Exited Unexpected : %s\n", strerror(errno));
	}

	rc2 = pthread_join(threadEi, NULL);
	if (rc2 != 0) {
		fprintf(stderr, "Thread 'threadEi' Exited Unexpected : %s\n", strerror(errno));
	}

	return;
}

void rkDamIoThread(void *handle)
{
	struct context *ctx = (struct context *)handle;

	while(1) {
		rkDamGetAiVal(&ctx->m_tAnalogParam);
		rkDamPushAiInfo(&ctx->m_tAnalogParam);
		rkDamGetDiVal(&ctx->m_tDioParam);
		rkDamPushDiInfo(ctx->m_tDioParam.m_tDiParam);
		rkDamSetDoVal(ctx->m_tDioParam.m_tDoParam);
		rkDamPushDoInfo(ctx->m_tDioParam.m_tDoParam);
		usleep(IO_SAMPLE_INTERVAL_MS * 1000);
	}

	return;
}

void rkDamEiThread(void *handle)
{
	int index, ret;
	void *lib;
	char *fpath;
	pthread_t thread[COM_NUM];
	struct context *ctx = (struct context *)handle;
	struct uart *uart = &ctx->m_tUartParam;
	struct com *com;

	for (index = 0; index < COM_NUM; index++) {
		thread[index] = -1;
		com = &uart->m_tComParam[index];
		fpath = (char *)rkLibGetPath(com->proto);
		if (!fpath) {
			continue;
		}

		lib = dlopen(fpath, RTLD_LAZY);
		if (!lib) {
			fprintf(stderr, "%s.\n", dlerror());
			continue;
		}

		if ((com->cfi.init = dlsym(lib, "init")) == NULL) {
			fprintf(stderr, "%s.\n", dlerror());
			dlclose(lib);
			continue;
		}
		com->cfi.init(NULL);

		if ((com->cfi.run = dlsym(lib, "run")) == NULL) {
			fprintf(stderr, "%s.\n", dlerror());
			dlclose(lib);
			continue;
		}

		ret = pthread_create(&thread[index], NULL, (void *)rkDamEiChildThread, com);
		if (ret < 0) {
			fprintf(stderr, "Can't create EI Child thread : %s.\n", strerror(ret));
			continue;
		}
	}

	for (index = 0; index < COM_NUM; index++) {
		if (thread[index] == 0) {
			pthread_join(thread[index], NULL);
		}
	}

	return;
}

int rkDamEiDataProc(struct edev *ei)
{
	int ret = 0;
	char tmp[64];
	float rawval, fineval;

	if (!strlen(ei->code)) {
		return -1;
	}

	/* Convert Raw Value */
	rawval = ei->vals[RTD_RAW_VAL_OFFSET];

	if (ei->usefml) {
		rkFmlUpdateSymTbl(FML_VAR_PREFIX, rawval);
		bzero(tmp, sizeof(tmp));
		ret = rkFmlConvExpr(ei->fml, tmp, ei->code);
		fineval = !ret ? rkFmlEvaluateExpr(tmp) : 0;
	} else {
		fineval = rawval;
	}

	/* Update Formula Symbol Table */
	sprintf(tmp, "%s%s", FML_VAR_PREFIX, ei->code);
	rkFmlUpdateSymTbl(tmp, fineval);

	/* Save Converted Value */
	ei->vals[RTD_CONV_VAL_OFFSET] = fineval;

	/* Calcuate Statistic */
	rkDamCalcStat(ei->code, &ei->stat, ei->vals[RTD_CONV_VAL_OFFSET]);

	return ret;
}

int rkDamPushEiInfo(struct edev *ei)
{
	struct rtdInfo rtd;

	rtd.id = ei->id;
	rtd.inuse = ei->inuse;
	strncpy(rtd.code, ei->code, sizeof(rtd.code) - 1);
	rtd.vals[RTV_OFFSET] = ei->vals[RTD_CONV_VAL_OFFSET];
	rtd.vals[TAV_OFFSET] = ei->stat.avg[MNT_DATA_STAT_OFFSET];
	rtd.vals[HAV_OFFSET] = ei->stat.avg[HOU_DATA_STAT_OFFSET];
	rtd.vals[DAV_OFFSET] = ei->stat.avg[DAY_DATA_STAT_OFFSET];
	rtd.vals[MAV_OFFSET] = ei->stat.avg[MON_DATA_STAT_OFFSET];
	rtd.alm = ei->alarm;
	rkShmPushData(TYPE_EI, ei->id, &rtd);

	return 0;
}

void rkDamEiChildThread(void *handle)
{
	int index, rc1, rc2; 
	int id = ((struct com*)handle)->id;
	struct uart *puart = &ctx.m_tUartParam;
	struct edev *ei;

	while(1) {
		for (index = 0; index < EI_NUM; index++) {
			ei = &puart->m_tChannelParam[index];
			if (ei->inuse && (ei->com == id)) {
				pthread_mutex_lock(&ei->mutex);
				rc1 = puart->m_tComParam[id].cfi.run(ei);
				rc2 = rkDamEiDataProc(ei);
				if (rc1 == 0 && rc2 == 0) {
					ei->alarm = ALARM_N;
					ei->flag = 'N';
				} else {
					ei->alarm = ALARM_E;
					ei->flag = 'D';
				}
				pthread_mutex_unlock(&ei->mutex);
			}
			rkDamPushEiInfo(ei);
		}
		usleep(EI_SAMPLE_INTERVAL_MS * 1000);
	}

	return;
}
