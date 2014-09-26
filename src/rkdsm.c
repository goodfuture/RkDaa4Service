/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.0
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkdsm.c
 * Created At		: 2013-09-23 14:29
 * Last Modified	: 2013-09-23 14:29
 * Description		: 
 * History			: 
*******************************************************************************/

#include "rkdsm.h"
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

/* Table Name */
static char *msgTblName[] = { RTM_TBL_NAME, MOM_TBL_NAME, HOM_TBL_NAME, DOM_TBL_NAME, DIM_TBL_NAME, AOM_TBL_NAME};

/* Table Capacity */
static int msgTblCap[] = { MAX_RTM_REC_NUM, MAX_MOM_REC_NUM, MAX_HOM_REC_NUM, MAX_DOM_REC_NUM, MAX_DIM_REC_NUM, MAX_AOM_REC_NUM};

static sqlite3 *rtddb, *msgdb;

void rkRiuThread(void *handle);
void rkRtdSaveThread(void *handle);

int rkDsmInit(struct context *ctx)
{
	char *sql, *errmsg, *path;
	int ret, i;

	/* Open & Create RTD Database */
	path = ctx->m_pCmdLineArg[ARG_RTD_DB_PATH_OFFSET];

	ret = sqlite3_open_v2(path, &rtddb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_SHAREDCACHE, NULL);
	if (ret) {
		fprintf(stderr, "Can't open or create database \"%s\".\n", path);
		return -1;
	}

	/* Open & Create MSG Database */
	path = ctx->m_pCmdLineArg[ARG_MSG_DB_PATH_OFFSET];
	ret = sqlite3_open_v2(path, &msgdb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_SHAREDCACHE, NULL);
	if (ret) {
		fprintf(stderr, "Can't open or create database \"%s\".\n", path);
		return -1;
	}

	/* Create RTD Table */
	sql = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS %q(Id INTEGER PRIMARY KEY, TimeStamp INTEGER, ChlNum TEXT, ChlCode TEXT, Value REAL);", RTD_TBL_NAME);
	ret = sqlite3_exec(rtddb, sql, NULL, NULL, &errmsg);
	sqlite3_free(sql);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "Can't create table \'%s\' : %s.\n", RTD_TBL_NAME, errmsg);
		return -1;
	}

	/* Create RI Table */
	sql = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS %q(Id INTEGER PRIMARY KEY, StartTime TEXT, StopTime TEXT);", RI_TBL_NAME);
	ret = sqlite3_exec(rtddb, sql, NULL, NULL, &errmsg);
	sqlite3_free(sql);
	if (ret != SQLITE_OK) {
		fprintf(stderr, "Can't create table \'%s\' : %s.\n", RI_TBL_NAME, errmsg);
		return -1;
	}

	/* Create Message Table */
	for (i = 0; i < sizeof(msgTblName) / sizeof(char *); i++) {
		sql = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS %q(Id INTEGER PRIMARY KEY, TimeStamp INTEGER, Msg TEXT);", msgTblName[i]);
		ret = sqlite3_exec(msgdb, sql, NULL, NULL, &errmsg);
		sqlite3_free(sql);
		if (ret != SQLITE_OK) {
			fprintf(stderr, "Can't create table \'%s\' : %s.\n", msgTblName[i], errmsg);
			return -1;
		}
	}

	return 0;
}

void rkDsmRun(void *handle)
{
	int ret;
	pthread_t riuThread, rtdSaveThread;
	struct context *ctx = (struct context *)handle;

	ret = pthread_create(&riuThread, NULL, (void *)rkRiuThread, rtddb);
	if (ret < 0) {
		fprintf(stderr, "Can't create RIU thread : %s.\n", strerror(errno));
		pthread_exit(&ret);
	}

	ret = pthread_create(&rtdSaveThread, NULL, (void *)rkRtdSaveThread, ctx);
	if (ret < 0) {
		fprintf(stderr, "Can't create RIU thread : %s.\n", strerror(errno));
		pthread_exit(&ret);
	}

	pthread_join(riuThread, NULL);
	pthread_join(rtdSaveThread, NULL);

	return;
}

/*
 * Runtime Information Update Thread
 */
void rkRiuThread(void *handle)
{
	sqlite3 *db = (sqlite3 *)handle;
	char *sql, tms[64], *errmsg;
	time_t ct;
	struct tm *cts;
	int rc;
	uint8_t isboot = 1;

	do {
		time(&ct);
		cts = localtime(&ct);
		bzero(tms, sizeof(tms));
		sprintf(tms, "%04d-%02d-%02d %02d:%02d:%02d", cts->tm_year + 1900, cts->tm_mon + 1, cts->tm_mday, cts->tm_hour, cts->tm_min, cts->tm_sec);
		if (!isboot) {
			sql = sqlite3_mprintf("UPDATE %q SET StopTime='%q' WHERE Id=(SELECT MAX(Id) FROM %q)", RI_TBL_NAME, tms, RI_TBL_NAME);
			rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
			sqlite3_free(sql);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "RUI thread : %s.\n", errmsg);
				continue;
			}
		} else {
			sql = sqlite3_mprintf("INSERT INTO %q VALUES(NULL,'%q','%q');", RI_TBL_NAME, tms, tms);
			rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
			sqlite3_free(sql);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "RIU thread : %s.\n", errmsg);
				pthread_exit(&rc);
			}
			isboot = 0;
		}

		/* Delete Oldest Record */
		sql = sqlite3_mprintf("DELETE FROM %q WHERE Id < (SELECT MAX(Id) FROM %q) - %d;", RI_TBL_NAME, RI_TBL_NAME, MAX_RI_REC_NUM);
		rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
		sqlite3_free(sql);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "RIU thread : %s.\n", errmsg);
			pthread_exit(&rc);
		}

		sleep(RUNTIME_INFO_UPDATE_INTERVAL_S);
	} while(1);

	return;
}

/* 
 * Real-Time Data Save Thread
 */
void rkRtdSaveThread(void *handle)
{
	struct context *ctx = (struct context *)handle;
	int i, rc;
	time_t tm;
	char chlNum[32], chlCode[32];
	char *sql, *errmsg;

	while(1) {
		time(&tm);
		for (i = 0; i < DI_NUM; i++) {
			struct dioparam *dip = &ctx->m_tDioParam.m_tDiParam[i];
			if (!dip->inuse) {
				continue;
			}
			bzero(chlNum, sizeof(chlNum));
			bzero(chlCode, sizeof(chlCode));
			sprintf(chlNum, "DI%d", i);
			strcpy(chlCode, dip->code);
			sql = sqlite3_mprintf("INSERT INTO %q VALUES (NULL, %ld, '%q', '%q', %f);", RTD_TBL_NAME, tm, chlNum, chlCode, (float)(dip->val));
			rc = sqlite3_exec(rtddb, sql, NULL, NULL, &errmsg);
			sqlite3_free(sql);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "RTD save thread : %s.\n", errmsg);
				continue;
			}
		}

		for (i = 0; i < DO_NUM; i++) {
			struct dioparam *dop = &ctx->m_tDioParam.m_tDoParam[i];
			if (!dop->inuse) {
				continue;
			}
			bzero(chlNum, sizeof(chlNum));
			bzero(chlCode, sizeof(chlCode));
			sprintf(chlNum, "DO%d", i);
			strcpy(chlCode, dop->code);
			sql = sqlite3_mprintf("INSERT INTO %q VALUES (NULL, %ld, '%q', '%q', %f);", RTD_TBL_NAME, tm, chlNum, chlCode, (float)(dop->val));
			rc = sqlite3_exec(rtddb, sql, NULL, NULL, &errmsg);
			sqlite3_free(sql);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "RTD save thread : %s.\n", errmsg);
				continue;
			}
		}

		for (i = 0; i < AI_NUM; i++) {
			struct aiparam *ai = &ctx->m_tAnalogParam.m_tChannelParam[i];
			if (!ai->inuse) {
				continue;
			}
			bzero(chlNum, sizeof(chlNum));
			bzero(chlCode, sizeof(chlCode));
			sprintf(chlNum, "AI%d", i);
			strcpy(chlCode, ai->code);
			sql = sqlite3_mprintf("INSERT INTO %q VALUES (NULL, %ld, '%q', '%q', %f);", RTD_TBL_NAME, tm, chlNum, chlCode, ai->vals[RTD_CONV_VAL_OFFSET]);
			rc = sqlite3_exec(rtddb, sql, NULL, NULL, &errmsg);
			sqlite3_free(sql);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "RTD save thread : %s.\n", errmsg);
				continue;
			}
		}

		for (i = 0; i < EI_NUM; i++) {
			struct edev *ei = &ctx->m_tUartParam.m_tChannelParam[i];
			if (!ei->inuse) {
				continue;
			}
			bzero(chlNum, sizeof(chlNum));
			bzero(chlCode, sizeof(chlCode));
			sprintf(chlNum, "EI%d", i);
			strcpy(chlCode, ei->code);
			sql = sqlite3_mprintf("INSERT INTO %q VALUES (NULL, %ld, '%q', '%q', %f);", RTD_TBL_NAME, tm, chlNum, chlCode, ei->vals[RTD_CONV_VAL_OFFSET]);
			rc = sqlite3_exec(rtddb, sql, NULL, NULL, &errmsg);
			sqlite3_free(sql);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "RTD save thread : %s.\n", errmsg);
				continue;
			}
		}

		/* Delete Oldest Records */
		sql = sqlite3_mprintf("DELETE FROM %q WHERE Id < (SELECT MAX(Id) From %q) - %d;", RTD_TBL_NAME, RTD_TBL_NAME, MAX_RTD_REC_NUM);
		rc = sqlite3_exec(rtddb, sql, NULL, NULL, &errmsg);
		sqlite3_free(sql);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "%s : %s.\n", __func__, errmsg);
			continue;
		}

		sleep(ctx->m_tSystemParam.dsi);
	}

	return;
}

/* 
 * Message Data Save Function
 */
int rkDsmSaveMsg(MSG_TYPE_T type, char *msg)
{
	time_t tm;
	time(&tm);
	char *sql, *errmsg;
	int rc;

	/* Insert Message To Table */
	sql = sqlite3_mprintf("INSERT INTO %q VALUES (NULL, %ld, '%q');", msgTblName[type], tm, msg);
	rc = sqlite3_exec(msgdb, sql, NULL, NULL, &errmsg);
	sqlite3_free(sql);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "%s : %s.\n", __func__, errmsg);
		return -1;
	}

	/* Delete Outdated Message Records From Table */
	sql = sqlite3_mprintf("DELETE FROM %q WHERE Id < (SELECT MAX(Id) FROM %q) - %d;", msgTblName[type], msgTblName[type], msgTblCap[type]);
	rc = sqlite3_exec(msgdb, sql, NULL, NULL, &errmsg);
	sqlite3_free(sql);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "%s : %s.\n", __func__, errmsg);
		return -1;
	}

	return 0;
}
