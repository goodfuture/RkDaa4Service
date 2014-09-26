/*******************************************************************************
 * Copyright(C)	    : 2013 Rockontrol Industrial Park, Inc.
 * Version          : V1.0
 * Author           : KangQi
 * Tel              : 18636181581/6581
 * Email            : goodfuturemail@gmail.com
 * File Name        : main.c
 * Created At       : 2013-09-23 17:46
 * Last Modified    : 2014-01-14 15:05
 * Description      : 
 * History	        : 
*******************************************************************************/
#include "rktype.h"
#include "rkxml.h"
#include "rkinit.h"
#include "rkdam.h"
#include "rkdtm.h"
#include "rkhmi.h"
#include "rkdsm.h"
#include "rkdebug.h"
#include "rkserver.h"
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

struct context ctx;

void sigHandler(int sig)
{
	printf("Program Exit.\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int ret;
	pthread_t damThread, dtmThread, dsmThread, serverThread, hmiThread;

	signal(SIGINT, sigHandler);

	ret = rkSysInit(&ctx, argc, argv);
	if (ret == -1) {
		fprintf(stderr, "System Initialize Failed, Program Exited.\n");
		return -1;
	}

	/* Data Acquisition */
	ret = pthread_create(&damThread, NULL, (void *)rkDamRun, &ctx);
	if (ret < 0) {
		fprintf(stderr, "Can't create DAM thread : %s.\n", strerror(errno));
		return -1;
	}

	/* Data Transmition */
	ret = pthread_create(&dtmThread, NULL, (void *)rkDtmRun, &ctx);
	if (ret < 0) {
		fprintf(stderr, "Can't create DTM thread : %s.\n", strerror(errno));
		return -1;
	}

	/* Data Storage */
	ret = pthread_create(&dsmThread, NULL, (void *)rkDsmRun, &ctx);
	if (ret < 0) {
		fprintf(stderr, "Can't create DSM thread : %s.\n", strerror(errno));
		return -1;
	}

	/* Tcp Server */
	ret = pthread_create(&serverThread, NULL, (void *)rkTcpServerThread, NULL);
	if (ret < 0) {
		fprintf(stderr, "Can't create Tcp Server thread : %s.\n", strerror(errno));
		return -1;
	}

	/* HMI */
	ret = pthread_create(&hmiThread, NULL, (void *)rkHmiRun, &ctx);
	if (ret < 0) {
		fprintf(stderr, "Can't create HMI thread : %s.\n", strerror(errno));
		return -1;
	}

	while(1) {
		if (ctx.exit) {
			printf("Program exit according to UI's command\n");
			exit(1);
		}
		usleep(100000);
	}

	return 0;
}
