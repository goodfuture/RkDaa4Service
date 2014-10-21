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

int rkDsmInit(struct context *ctx)
{
	sqlite3 *db;
	char *sql, *errmsg, *path;
	int ret, i;

	/* Open & Create RTD Database */
	path = ctx->m_pCmdLineArg[ARG_RTD_DB_PATH_OFFSET];
	ret = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, NULL);
	if (ret != SQLITE_OK) {
		ret = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_SHAREDCACHE, NULL);
		if (ret == SQLITE_OK) {
			/* Create RTD Table */
			sql = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS %q(Id INTEGER PRIMARY KEY, TimeStamp INTEGER, ChlNum TEXT, ChlCode TEXT, Value REAL);", RTD_TBL_NAME);
			ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
			if (ret != SQLITE_OK) {
				fprintf(stderr, "Can't create table \'%s\' : %s.\n", RTD_TBL_NAME, errmsg);
			}
			sqlite3_free(sql);

			/* Create RI Table */
			sql = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS %q(Id INTEGER PRIMARY KEY, StartTime TEXT, StopTime TEXT);", RI_TBL_NAME);
			ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
			if (ret != SQLITE_OK) {
				fprintf(stderr, "Can't create table \'%s\' : %s.\n", RI_TBL_NAME, errmsg);
			}
			sqlite3_free(sql);
		} else {
			fprintf(stderr, "Can't open or create database \"%s\".\n", path);
			return -1;
		}
	}
	sqlite3_close(db);

	/* Open & Create MSG Database */
	path = ctx->m_pCmdLineArg[ARG_MSG_DB_PATH_OFFSET];
	ret = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, NULL);
	if (ret != SQLITE_OK) {
		ret = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_SHAREDCACHE, NULL);
		if (ret == SQLITE_OK) {
			/* Create Message Table */
			for (i = 0; i < sizeof(msgTblName) / sizeof(char *); i++) {
				sql = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS %q(Id INTEGER PRIMARY KEY, TimeStamp INTEGER, Msg TEXT);", msgTblName[i]);
				ret = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
				if (ret != SQLITE_OK) {
					fprintf(stderr, "Can't create table \'%s\' : %s.\n", msgTblName[i], errmsg);
				}
				sqlite3_free(sql);
			}
		} else {
			fprintf(stderr, "Can't open or create database \"%s\".\n", path);
			return -1;
		}
	}
	sqlite3_close(db);

	return 0;
}

void rkDsmRun(void *handle)
{
	int i, rc;
	time_t tm;
	struct tm *ctm;
	sqlite3 *db;
	uint8_t isboot = 1;
	char *sql, *errmsg, *path, buf[32];
	char chlNum[32], chlCode[32];
	struct context *ctx = (struct context *)handle;
	
	path = ctx->m_pCmdLineArg[ARG_RTD_DB_PATH_OFFSET];

	while(1) {
		time(&tm);
		ctm = localtime(&tm);

		/* Open Realtime Value Database */
		rc = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, NULL);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "Can't open database \'%s\'.\n", path);
			sqlite3_close(db);
			continue;
		}

		/* Update Run Record */
		sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", ctm->tm_year + 1900, ctm->tm_mon + 1, ctm->tm_mday, ctm->tm_hour, ctm->tm_min, ctm->tm_sec);
		if (!isboot) {
			sql = sqlite3_mprintf("UPDATE %q SET StopTime='%q' WHERE Id=(SELECT MAX(Id) FROM %q)", RI_TBL_NAME, buf, RI_TBL_NAME);
			rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "RUI thread : %s.\n", errmsg);
			}
			sqlite3_free(sql);
		} else {
			sql = sqlite3_mprintf("INSERT INTO %q VALUES(NULL,'%q','%q');", RI_TBL_NAME, buf, buf);
			rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "RIU thread : %s.\n", errmsg);
			}
			sqlite3_free(sql);
			isboot = 0;
		}

		/* Delete Oldest Record */
		sql = sqlite3_mprintf("DELETE FROM %q WHERE Id < (SELECT MAX(Id) FROM %q) - %d;", RI_TBL_NAME, RI_TBL_NAME, MAX_RI_REC_NUM);
		rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "RIU thread : %s.\n", errmsg);
		}
		sqlite3_free(sql);

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
			rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "RTD save thread : %s.\n", errmsg);
			}
			sqlite3_free(sql);
		}

		/* Update Realtime Value Database */
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
			rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "RTD save thread : %s.\n", errmsg);
			}
			sqlite3_free(sql);
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
			rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "RTD save thread : %s.\n", errmsg);
			}
			sqlite3_free(sql);
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
			rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "RTD save thread : %s.\n", errmsg);
			}
			sqlite3_free(sql);
		}

		/* Delete Oldest Records */
		sql = sqlite3_mprintf("DELETE FROM %q WHERE Id < (SELECT MAX(Id) From %q) - %d;", RTD_TBL_NAME, RTD_TBL_NAME, MAX_RTD_REC_NUM);
		rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "%s : %s.\n", __func__, errmsg);
		}
		sqlite3_free(sql);

		sqlite3_close(db);

		sleep(ctx->m_tSystemParam.dsi);
	}

	return;
}

/* 
 * Message Data Save Function
 */
int rkDsmSaveMsg(MSG_TYPE_T type, char *msg)
{
	int rc;
	time_t tm;
	sqlite3 *db;
	char sql[2048], *errmsg, *path;

	time(&tm);

	/* Open MSG Database */
	path = ctx.m_pCmdLineArg[ARG_MSG_DB_PATH_OFFSET];
	rc = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, NULL);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Can't open database \'%s\'.\n", path);
		sqlite3_close(db);
		return -1;
	}

	/* Insert Message To Table */
#if 0
	sql = sqlite3_mprintf("INSERT INTO %q VALUES (NULL, %ld, '%q');", msgTblName[type], tm, msg);
	rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "%s : %s.\n", __func__, errmsg);
	}
	sqlite3_free(sql);
#else
	bzero(sql, sizeof(sql));
	sprintf(sql, "INSERT INTO %s VALUES (NULL, %ld, '%s');", msgTblName[type], tm, msg);
	rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "%s : %s.\n", __func__, errmsg);
	}
#endif

#if 0
	/* Delete Outdated Message Records From Table */
	sql = sqlite3_mprintf("DELETE FROM %q WHERE Id < (SELECT MAX(Id) FROM %q) - %d;", msgTblName[type], msgTblName[type], msgTblCap[type]);
	rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "%s : %s.\n", __func__, errmsg);
	}
	sqlite3_free(sql);
#else
	bzero(sql, sizeof(sql));
	sprintf(sql, "DELETE FROM %s WHERE Id < (SELECT MAX(Id) FROM %s) - %d;", msgTblName[type], msgTblName[type], msgTblCap[type]);
	rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "%s : %s.\n", __func__, errmsg);
	}
#endif

	sqlite3_close(db);

	return 0;
}
