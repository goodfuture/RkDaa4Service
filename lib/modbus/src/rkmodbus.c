#include "rkmodbus.h"
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

int rkGenModbusMsg03(char *buf, uint8_t slvaddr, uint16_t regaddr, uint8_t datatype)
{
	uint16_t crc16;

	buf[0] = slvaddr;
	buf[1] = 0x03;
	buf[2] = regaddr >> 8;
	buf[3] = regaddr & 0xff;
	buf[4] = 0x00;
	if (datatype == 0) {
		buf[5] = 0x01;
	} else {
		buf[5] = 0x02;
	}

	crc16 = rkCrc16(buf, 6);
	buf[6] = crc16 >> 8;
	buf[7] = crc16 & 0xff;

	return 8;
}

int rkSendModbusReq(int fd, const char *sendbuf, int bufsize)
{
	return rkSerSend(sendbuf, bufsize, fd);
}

int rkRecvModbusResp03(char *buf, uint8_t slvaddr, uint16_t regaddr, int fd)
{
	int pktlen; 
	unsigned int recvcnt = 0;
	uint16_t cal_crc, pkt_crc;

	while(rkSerRecv(&buf[recvcnt++], 1, MB_TIME_OUT_MS, fd) > 0) {
		if (recvcnt == 1 && buf[0] != slvaddr) {
			recvcnt = 0;
		} else if (recvcnt == 2 && buf[1] != 0x03) {
			recvcnt = 0;
		} else if (recvcnt == 3) {
			pktlen = recvcnt + buf[2] + 2;
		} else if (recvcnt == pktlen) {
			cal_crc = rkCrc16(buf, recvcnt - 2);
			pkt_crc = buf[recvcnt - 2] << 8 | buf[recvcnt - 1];

			if (cal_crc == pkt_crc) {
				return recvcnt;
			} else {
				return -1;
			}
		} 
	}

	return -1;
}

int rkMbProcResp(char *buf, float *val)
{
	uint8_t	offset = 0, len;
	union {
		float	value_f;
		uint8_t value_c[4];
	} value;

	switch(buf[1]) {
		case 0x03: 
			offset = 2;
			len = buf[offset++];
			if (len == 0x02) {
				value.value_f = (float)(buf[offset] << 8 | buf[offset + 1]);
			} else if (len == 0x04) {
				value.value_c[1] = buf[offset++];
				value.value_c[0] = buf[offset++];
				value.value_c[3] = buf[offset++];
				value.value_c[2] = buf[offset++];
			} else {
				return -1;
			}

			*val = value.value_f;
			break;
		default:
			break;
	}

	return 0;
}

int init(void *handle)
{
	//struct com *com = (struct com *)handle;

	return 0;
}

char *name(void)
{
	return "modbus";
}

int run(void *handle)
{
	int ret;
	MB_STEP_T state = MB_REQUEST;
	struct edev *edev = (struct edev *)handle;
	char buf[128];

	while(state != MB_FINISH) {
		switch(state) {
			case MB_REQUEST:
				bzero(buf, sizeof(buf));
				ret = rkGenModbusMsg03(buf, (uint8_t)edev->devid, (uint16_t)edev->regaddr, (uint8_t)edev->datatype);
				if (rkSendModbusReq(edev->fd, buf, ret) != ret) {
					state = MB_ERROR;
				} else {
					state = MB_RECEIVE;
				}
				break;
			case MB_RECEIVE:
				bzero(buf, sizeof(buf));
				if (rkRecvModbusResp03(buf, (uint8_t)edev->devid, (uint16_t)edev->regaddr, edev->fd) < 0) {
					state = MB_ERROR;
				} else {
					state = MB_PROCESS;
				}
				break;
			case MB_PROCESS:
				rkMbProcResp(buf, &edev->vals[RTD_RAW_VAL_OFFSET]);
				state = MB_FINISH;
				break;
			case MB_FINISH:
				break;
			case MB_ERROR:
				return -1;
		}
	}

	return 0;
}
