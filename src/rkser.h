#ifndef _RK_SERIAL_H_
#define _RK_SERIAL_H_

#include "rktype.h"

/* 
 * Function : open uart with specified configuration 
 * Parameter : 
 * Ret : if success return it's file descriptor Or return -1
 */
int rkSerOpen(struct com *com);

int rkSerClose(int fd);

int rkSerRecv(char *buf, uint32_t len, int timeout, int fd);

/*
 * Function : send data from uart
 * Parameter : timeout - ms
 * Ret : on success, return the number of characters sent, on error -1 is returned.
 */
int rkSerSend(const char *buf, uint32_t len, int fd);

#endif /* _RK_SERIAL_H_ */
