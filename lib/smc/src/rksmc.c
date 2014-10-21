#include "rksmc.h"
#include "rkcrc.h"
#include "rkser.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

int printHex(void *buf, int len)
{
	int cnt;

	for (cnt = 1; cnt <= len; cnt++) {
		printf("%02X ", ((unsigned char *)buf)[cnt - 1]);
		if (cnt % 16 == 0) {
			printf("\n");
		}   
	}   

	if ((cnt - 1) % 16) {
		printf("\n");
	}   

	return 0;
} 

int rkGenSmcMsg(char *buf, uint8_t slvaddr, uint16_t regaddr, uint8_t datatype)
{
	printf("inside = 0x%X\n", regaddr);
	buf[0] = '>';
	buf[1] = '*';
	buf[2] = 0x07;
	buf[3] = 0xD1;
	buf[4] = regaddr >> 8;
	buf[5] = regaddr & 0xFF;
	buf[6] = 0x00;
	buf[7] = 0x01;

	return 8;
}

int rkSendSmcReq(int fd, const char *sendbuf, int bufsize)
{
	return rkSerSend(sendbuf, bufsize, fd);
}

int rkRecvSmcResp(char *buf, float *val, int fd)
{
	int datalen = 0; 
	unsigned int recvcnt = 0;
	union {
		float	value_f;
		uint8_t value_c[4];
	} value;

	while(rkSerRecv(&buf[recvcnt++], 1, SMC_TIME_OUT_MS, fd) > 0) {
		if (recvcnt == 1 && buf[0] != '<') {
			recvcnt = 0;
		} else if (recvcnt == 2 && buf[1] != '*') {
			recvcnt = 0;
		} else if (recvcnt == 3 && buf[2] != 0x07) {
			recvcnt = 0;
		} else if (recvcnt == 4 && buf[3] != 0xD1) {
			recvcnt = 0;
		} else if (recvcnt == 6) {
			datalen = buf[4] + buf[5];
		} else if (recvcnt == datalen + 6) {
#if 0
			value.value_c[1] = buf[6];
			value.value_c[0] = buf[7];
			value.value_c[3] = buf[8];
			value.value_c[2] = buf[9];
#else
			value.value_c[0] = buf[6];
			value.value_c[1] = buf[7];
			value.value_c[2] = buf[8];
			value.value_c[3] = buf[9];
#endif
			*val = value.value_f;
			return 0;
		} 
	}


	return -1;
}

int init(void *handle)
{
	return 0;
}

char *name(void)
{
	return "smc";
}

int run(void *handle)
{
	int ret;
	SMC_STEP_T state = SMC_REQUEST;
	struct edev *edev = (struct edev *)handle;
	char buf[128];

	while(state != SMC_FINISH) {
		switch(state) {
			case SMC_REQUEST:
				bzero(buf, sizeof(buf));
				printf("outside = 0x%X\n", edev->regaddr);
				ret = rkGenSmcMsg(buf, (uint8_t)edev->devid, (uint16_t)edev->regaddr, (uint8_t)edev->datatype);
				if (rkSendSmcReq(edev->fd, buf, ret) != ret) {
					state = SMC_ERROR;
				} else {
					state = SMC_RECEIVE;
				}
				break;
			case SMC_RECEIVE:
				bzero(buf, sizeof(buf));
				ret = rkRecvSmcResp(buf, &edev->vals[RTD_RAW_VAL_OFFSET], edev->fd);
				if (ret == -1) {
					state = SMC_ERROR;
				} else {
					state = SMC_FINISH;
				}
				break;
			case SMC_ERROR:
				return -1;
			case SMC_FINISH:
			default:
				break;
		}
	}

	return 0;
}
