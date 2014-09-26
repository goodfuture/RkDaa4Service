#ifndef _RK_FIFO_H_
#define _RK_FIFO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <semaphore.h>

typedef struct fifohdr {
	unsigned int size;
	unsigned int readOps;
	unsigned int writeOps;
	sem_t sem;
} fifohdr_t;

typedef struct fifo {
	struct fifohdr *hdr;
	char *buffer;
} fifo_t;

void rkFifoInit(struct fifo *, void *addr, unsigned int size);

unsigned int rkFifoWrt(struct fifo *, char *buf, unsigned int len);

unsigned int rkFifoRd(struct fifo *, char *buf, unsigned int len);

void rkFifoClr(struct fifo *fifo);

unsigned int rkFifoGetLen(struct fifo *fifo);

#ifdef __cplusplus
}
#endif

#endif /* _RK_FIFO_H_ */
