/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.1
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkshm.c
 * Created At		: 2013-09-23 10:01
 * Last Modified	: 2013-11-12 13:35
 * Description		: 
 * History			: 
 *******************************************************************************/

#include "rkcom.h"
#include "rkshm.h"
#include "rkfifo.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static struct fifo fifo;
static struct shmMap *shmmap;

int rkShmInit()
{
	int fd = shm_open(SHM_DEV_FILE, O_RDWR | O_CREAT, 0666);
	if (fd == -1) {
		return -1;
	}

	if (ftruncate(fd, SHM_SIZE) == -1) {
		close(fd);
		return -1;
	}

	shmmap = (struct shmMap *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (shmmap == MAP_FAILED) {
		close(fd);
		return -1;
	}
	memset(shmmap, 0, sizeof(SHM_SIZE));
	close(fd);

	rkFifoInit(&fifo, (char *)shmmap + FIFO_OFFSET, FIFO_SIZE);

	return 0;
}

int rkShmDel()
{
	return shm_unlink(SHM_DEV_FILE);
}

int rkIsReloadFlagBeenSet()
{
	return shmmap->flag & RELOAD_USR_CFG_BIT_MASK;
}

void rkClearReloadFlag()
{
	shmmap->flag &= ~RELOAD_USR_CFG_BIT_MASK;
}

void rkSetReloadFlag()
{
	shmmap->flag |= RELOAD_USR_CFG_BIT_MASK;
}

int rkIsReqExit()
{
	return shmmap->flag & CORE_PROG_EXIT_BIT_MASK;
}

void rkReqCoreExit()
{
	shmmap->flag |= CORE_PROG_EXIT_BIT_MASK;
}

void rkClearExitReq()
{
	shmmap->flag &= ~CORE_PROG_EXIT_BIT_MASK;
}

void rkDisableDtu(void)
{
	shmmap->flag |= DISABLE_DTU_BIT_MASK;
	//printf("shmmap->flag = 0x%02X\n", shmmap->flag);
}

int rkIsDtuDisabled()
{
	//printf("shmmap->flag = 0x%02X, shmmap->flag & DISABLE_DTU_BIT_MASK = %d\n", shmmap->flag, shmmap->flag & DISABLE_DTU_BIT_MASK);
	return shmmap->flag & DISABLE_DTU_BIT_MASK;
}

int rkIsDtuEnable()
{
	//printf("shmmap->flag = 0x%02X\n", shmmap->flag);
	return !(shmmap->flag & DISABLE_DTU_BIT_MASK);
}

void rkEnableDtu(void)
{
	shmmap->flag &= ~DISABLE_DTU_BIT_MASK;
}

int rkCoreBeatOnce(const char *ver)
{
	if (strlen(ver) >= 8) {
		return -1;
	}

	rkPutCoreVersion(ver);

	return	++shmmap->heartbeat;
}

int rkIsCoreAlive()
{
	static uint32_t	 lastBeat = 0;

	if (shmmap->heartbeat == lastBeat) {
		return 0;
	} else {
		lastBeat = shmmap->heartbeat;
		return 1;
	}
}

void rkPutCoreVersion(const char *version)
{
	//memset(shmmap->version[1], 0, 8);
	memcpy(shmmap->version[1], version, strlen(version));
}

void rkPutGuiVersion(const char *version)
{
	//memset(shmmap->version[0], 0, 8);
	memcpy(shmmap->version[0], version, strlen(version));
}

const char *rkGetCoreVersion()
{
	return strlen(shmmap->version[1]) >= 8 ? NULL : shmmap->version[1];
}

const char *rkGetGuiVersion()
{
	return strlen(shmmap->version[0]) >= 8 ? NULL : shmmap->version[0];
}

int rkShmPushData(uint8_t type, uint8_t index, void *val)
{
	if (type == TYPE_AI) { /* AI */
		memcpy(&shmmap->ai[index], val, sizeof(struct rtdInfo));
	} else if (type == TYPE_EI) { /* EI */
		memcpy(&shmmap->ei[index], val, sizeof(struct rtdInfo));
	} else if (type == TYPE_DI) { /* DI */
		memcpy(&shmmap->dip[index], val, sizeof(struct dioInfo));
	} else if (type == TYPE_DO) { /* DO */
		memcpy(&shmmap->dop[index], val, sizeof(struct dioInfo));
	} else {
		return -1;
	}

	return 0;
}

int rkShmGetData(uint8_t type, uint8_t index, void *val)
{
	if (type == TYPE_AI) { /* AI */
		memcpy(val, &shmmap->ai[index], sizeof(struct rtdInfo));
	} else if (type == TYPE_EI) { /* EI */
		memcpy(val, &shmmap->ei[index], sizeof(struct rtdInfo));
	} else if (type == TYPE_DI) { /* DI */
		memcpy(val, &shmmap->dip[index], sizeof(struct dioInfo));
	} else if (type == TYPE_DO) { /* DO */
		memcpy(val, &shmmap->dop[index], sizeof(struct dioInfo));
	} else {
		return -1;
	}

	return 0;
}

int rkShmPutDataByFifo(char *data, unsigned int len)
{
	return rkFifoWrt(&fifo, data, len);
}

int rkShmGetDataByFifo(char *buf, unsigned int size)
{
	return rkFifoRd(&fifo, buf, size);
}

int rkShmQuery(queryCond_t *cond, rkRec_t **result)
{
	int size;
	rkRec_t *record;

	/* Check Previous Query Session State */
	if (shmmap->dataReq.ask) { 
		return -1;
	}

	memcpy(&shmmap->dataReq.cond, cond, sizeof(struct queryCond));
	shmmap->dataReq.ask = 1; 

	/* Waiting For Query Result */
	do {
		usleep(10000);
	} while(!shmmap->dataReq.ready);

	/* Begine To Receive Result */
	record = (rkRec_t *)malloc(sizeof(rkRec_t));
	bzero(record, sizeof(rkRec_t));
	size = sizeof(record->buf);
	record->len = rkShmGetDataByFifo(record->buf, size);
	record->next = NULL;
	*result = record;
	while ( (int)record->len == size) {
		record->next = (rkRec_t *)malloc(sizeof(rkRec_t));
		record = record->next;
		bzero(record, sizeof(*record));
		record->len = rkShmGetDataByFifo(record->buf, size);
		record->next = NULL;
	}


	/* Finish This Query Session */
	shmmap->dataReq.ready = 0; 

	return 0;
}

void rkShmClrResult(rkRec_t *record)
{
	while(record) {
		rkRec_t *ptr = record->next;
		free(record);
		record = ptr;
	}
}

int rkShmListenReq(struct queryCond *cond)
{
	while(1) {
		if (shmmap->dataReq.ask) {
			memcpy(cond, &shmmap->dataReq.cond, sizeof(struct queryCond));
			return 1;
		}
		usleep(5000);
	}
}

void rkShmEchoReq(char *result, int size)
{
	int ret = 0;

	do {
		ret = rkShmPutDataByFifo(result, size);
		shmmap->dataReq.ready = 1;
		result += ret;
		size -= ret;
		usleep(10000);
	} while(size != 0);

	shmmap->dataReq.ask = 0;
}

