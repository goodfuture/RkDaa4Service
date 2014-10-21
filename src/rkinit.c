/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.0
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkinit.c
 * Created At		: 2013-09-23 14:29
 * Last Modified	: 2013-09-30 11:29
 * Description		: 
 * History			: 
*******************************************************************************/

#include "rkinit.h"
#include "rkdam.h"
#include "rkdtm.h"
#include "rkdsm.h"
#include "rkcou.h"
#include "rkhmi.h"
#include "rkdebug.h"
#include "rkcalib.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/types.h>

static const char *helpString = 
"Usage :\n"
" %s [options]\n"
"Options :\n"
" -A            Execute Analog Calibrate Program.\n"
" -C <dir>      Specify the configuation directory by 'dir'.\n"
"               Default use '%s'.\n"
" -R <path>     Specify the path of rtd database by 'path'.\n"
"               Default use '%s'.\n"
" -M <path>     Specify the path of msg database by 'path'.\n"
"               Default use '%s'.\n"
" -L <dir>      Specify the dir of dynamic extension library by 'dir'.\n"
"               Default use '%s'.\n"
" -V            Show program version.\n"
" -d            Run program with debug mode.\n"
" -h | -H       Get this help information.\n\n";

static int rkParseUsrCfg(struct context *ctx)
{
	rkXmlSetWorkDir(ctx->m_pCmdLineArg[ARG_CFG_DIR_OFFSET]);
	rkXmlParseSysParam(&ctx->m_tSystemParam);
	rkXmlParseNetParam(&ctx->m_tNetParam);
	rkXmlParseUartParam(&ctx->m_tUartParam);
	rkXmlParseDioParam(&ctx->m_tDioParam);
	rkXmlParseAnalogParam(&ctx->m_tAnalogParam);
	//printuart(ctx->m_tUartParam, 0);

	return 0;
}

int rkReloadUsrCfg(struct context *ctx)
{
	return rkParseUsrCfg(ctx);
}

static int rkParseCmdLineArg(struct context *ctx, int argc, char *argv[])
{
	int opt;
	while((opt = getopt(argc, argv, "C:R:M:L:AvVHhd")) != -1) {
		switch(opt) {
			case 'A':
				rkCliCalib();
				exit(0);
			case 'C': /* Configure Directory */
				ctx->m_pCmdLineArg[ARG_CFG_DIR_OFFSET] = strdup(optarg);
				break;
			case 'R': /* RTD Database Storage Path */
				ctx->m_pCmdLineArg[ARG_RTD_DB_PATH_OFFSET] = strdup(optarg);
				break;
			case 'M': /* MSG Database Storage Path */
				ctx->m_pCmdLineArg[ARG_MSG_DB_PATH_OFFSET] = strdup(optarg);
				break;
			case 'L': /* Path Of Dynamic Extension Library */
				ctx->m_pCmdLineArg[ARG_DYN_LIB_OFFSET] = strdup(optarg);
				break;
			case 'v':
			case 'V': /* Program Version */
				printf("version : %s\n", CORE_VERSION);
				exit(1);
			case 'd': /* Debug Mode */
				ctx->debug = 1;
				break;
			case 'H':
			case 'h':
				printf(helpString, argv[0], DEF_CFG_FILE_DIR, DEF_RTD_DB_PATH, DEF_MSG_DB_PATH, DEF_DYN_LIB_DIR);
				exit(1);
			default:
				printf("Use -h for a list of options.\n");
				exit(0);
		}
	}

	if (!ctx->m_pCmdLineArg[ARG_CFG_DIR_OFFSET]) {
		ctx->m_pCmdLineArg[ARG_CFG_DIR_OFFSET] = DEF_CFG_FILE_DIR;
	}

	if (!ctx->m_pCmdLineArg[ARG_RTD_DB_PATH_OFFSET]) { 
		ctx->m_pCmdLineArg[ARG_RTD_DB_PATH_OFFSET] = DEF_RTD_DB_PATH;
	}

	if (!ctx->m_pCmdLineArg[ARG_MSG_DB_PATH_OFFSET]) { 
		ctx->m_pCmdLineArg[ARG_MSG_DB_PATH_OFFSET] = DEF_MSG_DB_PATH;
	}
	if (!ctx->m_pCmdLineArg[ARG_DYN_LIB_OFFSET]) { 
		ctx->m_pCmdLineArg[ARG_DYN_LIB_OFFSET] = DEF_DYN_LIB_DIR;
	}


	return 0;
}

int rkSysInit(struct context *ctx, int argc, char *argv[])
{
	bzero(ctx, sizeof(struct context));

	/* Parse Command Line Arguments */
	if (rkParseCmdLineArg(ctx, argc, argv) == -1) {
		return -1;
	}

	rkInitConfigParam(ctx);

	/* Parse Xml Configuration files */
	if (rkParseUsrCfg(ctx) == -1) {
		return -1;
	}

	/* Init CodeMap Table */
	rkCouInitCTbl(&ctx->m_tCodeTable);

	/* Data Acquisition Module */
	if (rkDamInit(ctx) == -1) {
		return -1;
	}

	/* Data Transmit Module */
	if (rkDtmInit(ctx) == -1) { 
		//return -1;
	}

	/* Data Storage Module */
	if (rkDsmInit(ctx) == -1) {
		return -1;
	}

	if (rkHmiInit() == -1) {
		return -1;
	}

	if (rkTcpSocketInit() == -1) {
		return -1;
	}

	return 0;
}

void rkInitConfigParam(struct context *ctx)
{
	rkInitSysParam(&ctx->m_tSystemParam);
	rkInitNetParam(&ctx->m_tNetParam);
	rkInitUartParam(&ctx->m_tUartParam);
	rkInitDioParam(&ctx->m_tDioParam);
	rkInitAnalogParam(&ctx->m_tAnalogParam);
	rkInitAiCalParam(&ctx->m_tAnalogParam);
	rkScanDynamicProtocol(ctx);

	memset(ctx->m_aVersion[0], 0, 8);
	memset(ctx->m_aVersion[1], 0, 8);
	memset(ctx->m_aVersion[2], 0, 8);
	sprintf(ctx->m_aVersion[1], "%s", CORE_VERSION);
}

int rkScanDynamicProtocol(struct context *ctx)
{
	DIR *dirPtr; 
	void *lib;
	char libPath[256], *libName;
	struct dirent *dir;
	char *(*func)(void);
	char *dirPath = ctx->m_pCmdLineArg[ARG_DYN_LIB_OFFSET];

	dirPtr = opendir(dirPath);
	if (!dirPtr) {
		fprintf(stderr, "%s : %s.\n", __func__, strerror(errno));
		return -1;
	}

	while((dir = readdir(dirPtr)) != NULL) {
		if (!strstr(dir->d_name, ".so")) {
			continue;
		}

		if (dirPath[strlen(dirPath)] == '/') {
			sprintf(libPath, "%s%s", dirPath, dir->d_name);
		} else {
			sprintf(libPath, "%s/%s", dirPath, dir->d_name);
		}

		lib = dlopen(libPath, RTLD_LAZY);
		if (!lib) {
			fprintf(stderr, "%s.\n", dlerror());
			continue;
		}

		func = (char *(*)(void))dlsym(lib, "name");
		if (!func) {
			fprintf(stderr, "%s.\n", dlerror());
			dlclose(lib);
			continue;
		}

		if ((libName = func()) != NULL) {
			if (strlen(ctx->m_aDynamicProtocolSet)) {
				strcat(ctx->m_aDynamicProtocolSet, ",");
				strcat(ctx->m_aDynamicProtocolSet, libName);
			} else {
				bzero(ctx->m_aDynamicProtocolSet, sizeof(ctx->m_aDynamicProtocolSet));
				sprintf(ctx->m_aDynamicProtocolSet, "%s", libName);
			}
		}
		dlclose(lib);
	}
	closedir(dirPtr);

	//printf("lib: %s\n", ctx->m_aDynamicProtocolSet);

	return 1;
}

void rkInitSysParam(struct sys *sys)
{
	sprintf(sys->mn, "12345678901234");
	sprintf(sys->pw, "123456");
	sys->st = 32;
	sys->dui = 300;
	sys->dum = DTU;
	sys->dsi = 60;
	sys->rduen = ON;
	sys->mduen = ON;
	sys->hduen = ON;
	sys->dduen = ON;
	sys->sduen = ON;
	sys->alarmen = ON;
}

void rkInitNetParam(struct net *net)
{
}

void rkInitUartParam(struct uart *uart)
{
	int cnt;
	for (cnt = 0; cnt < COM_NUM; cnt++) {
		struct com *com = &uart->m_tComParam[cnt];
		com->id = cnt;
		com->baud = 9600;
		com->db = 8;
		com->sb = 1;
		com->parity = NONE;
		sprintf(com->proto, "none");
	}

	for (cnt = 0; cnt < EI_NUM; cnt++) {
		struct edev *ei = &uart->m_tChannelParam[cnt];
		ei->id = cnt;
	}
}

void rkInitDioParam(struct dio *dio)
{
	int cnt;
	for (cnt = 0; cnt < DI_NUM; cnt++) {
		struct dioparam *dip = &dio->m_tDiParam[cnt];
		dip->id = cnt;
	}

	for (cnt = 0; cnt < DO_NUM; cnt++) {
		struct dioparam *dop = &dio->m_tDoParam[cnt];
		dop->id = cnt;
	}
}

void rkInitAnalogParam(struct analog *analog)
{
	int cnt;
	for (cnt = 0; cnt < AI_NUM; cnt++) {
		struct aiparam *ai = &analog->m_tChannelParam[cnt];
		ai->id = cnt;
	}
}

void rkInitAiCalParam(struct analog *analog)
{
}
