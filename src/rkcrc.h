#ifndef _RK_CRC_H_
#define _RK_CRC_H_

#include "rktype.h"

uint16_t rkCrc16(char *buf, uint16_t len);
uint16_t rkCrc16New(char *src, uint16_t len);

#endif /* _RK_CRC_H_ */
