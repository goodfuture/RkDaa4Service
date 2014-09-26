/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.0
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkfifo.c
 * Created At		: 2013-09-23 14:27
 * Last Modified	: 2013-09-23 14:27
 * Description		: 
 * History			: 
*******************************************************************************/

#include "rkfifo.h"
#include <string.h>
#include <stdio.h>

#define ALIGN(x,a) (((x) + (a) - 1) & ~((a) - 1))
#define min(x,y) ({typeof(x) _x = (x); typeof(y) _y = (y); (void)(&_x == &_y); _x < _y ? _x : _y;})
#define max(x,y) ({typeof(x) _x = (x); typeof(y) _y = (y); (void)(&_x == &_y); _x > _y ? _x : _y;})

void rkFifoInit(struct fifo *fifo, void *addr, unsigned int size)
{
	/* Setting User Space Variable */
	fifo->hdr = (struct fifohdr *)addr;
	fifo->buffer = (char *)addr + sizeof(struct fifohdr);

	/* Change Share Memory Setting */
	fifo->hdr->size = size - sizeof(struct fifohdr);
	fifo->hdr->readOps = 0;
	fifo->hdr->writeOps = 0;
	sem_init(&fifo->hdr->sem, 1, 1);
}

unsigned int rkFifoWrt(struct fifo *fifo, char *buf, unsigned int len)
{
	sem_wait(&fifo->hdr->sem);
	unsigned int l;

	/* Calculate Free Space, And Select Small One Between 'len' And Free Space */
	len = min(len, fifo->hdr->size - fifo->hdr->writeOps + fifo->hdr->readOps);

	/* Begine To Copy */
	l = min(len, fifo->hdr->size - ( fifo->hdr->writeOps % fifo->hdr->size ));

	/* Copy From 'writeOps' To Buffer End */
	memcpy(fifo->buffer + (fifo->hdr->writeOps % fifo->hdr->size), buf, l);

	/* Copy From Buffer Begine To 'buf' End */
	memcpy(fifo->buffer, buf + l, len - l);

	/* Change Write Pointer */
	fifo->hdr->writeOps += len;
	sem_post(&fifo->hdr->sem);

	return len;
}

unsigned int rkFifoRd(struct fifo *fifo, char *buf, unsigned int len)
{
	sem_wait(&fifo->hdr->sem);
	unsigned int l;

	len = min(len, fifo->hdr->writeOps - fifo->hdr->readOps);

	l = min(len, fifo->hdr->size - (fifo->hdr->readOps % fifo->hdr->size));

	memcpy(buf, fifo->buffer + (fifo->hdr->readOps % fifo->hdr->size), l);

	memcpy(buf + l, fifo->buffer, len - l);

	fifo->hdr->readOps += len;
	sem_post(&fifo->hdr->sem);

	return len;
}

void rkFifoClr(struct fifo *fifo)
{
	fifo->hdr->readOps = fifo->hdr->writeOps = 0;
}

unsigned int rkFifoGetLen(struct fifo *fifo)
{
	return fifo->hdr->writeOps - fifo->hdr->readOps;
}
