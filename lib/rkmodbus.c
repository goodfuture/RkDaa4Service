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

int rkGenModbusMsg03(char *buf, unsigned int bufsize, uint8_t slvaddr, uint16_t regaddr, uint8_t datatype)
{
	uint16_t crc16;

	if (!buf || bufsize < 8) return -1;
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

static int rkSendModbusReq(int fd, const char *sendbuf, int bufsize)
{
	return rkSerSend(sendbuf, bufsize, fd);
}

static int rkRecvModbusResp(char *buf, uint8_t slvaddr, uint16_t regaddr, int fd)
{
	int needrecv; 
	unsigned int recvcnt = 0;
	uint16_t cal_crc, pkt_crc;
	//printf("slvaddr = %02X, regaddr = %04X\n", slvaddr, regaddr);
#if 1
	while(rkSerRecv(&buf[recvcnt++], 1, MB_TIME_OUT_MS, fd) > 0) {
		//printf("buf[%d] = %02X\n", recvcnt - 1, buf[recvcnt - 1]);
		if (recvcnt == 1 && buf[0] != slvaddr) {
			recvcnt = 0;
			continue;
		}
		//puts("*");

		if (recvcnt == 2 && buf[1] != 0x03) {
			recvcnt = 0;
			continue;
		}
		//puts("**");

		if (recvcnt == 3) {
			//puts("***");
			needrecv = buf[2] + 2;
			if (rkSerRecv(&buf[recvcnt], needrecv, MB_TIME_OUT_MS, fd) != needrecv) {
				return -1;
			}
			recvcnt += needrecv;
#if 0
			printf("recvcnt = %d\n", recvcnt);
			printHex(buf, recvcnt);
#endif

			cal_crc = rkCrc16(buf, recvcnt - 2);
			pkt_crc = buf[recvcnt - 2] << 8 | buf[recvcnt - 1];
#if 0
			printf("CAL_CRC = %04X, PKT_CRC = %04X\n", cal_crc, pkt_crc);
#endif
			if (cal_crc == pkt_crc) {
				return recvcnt;
			} else {
				return -1;
			}
		}
	}
#else 
	return rkSerRecv(buf, 256, MB_TIME_OUT_MS, fd);
#endif
	//puts("ERROR");

	return -1;
}

static int rkMbProcResp(char *buf, uint8_t slvaddr, float *val)
{
	uint8_t	function, offset = 0, len;
	union {
		float	value_f;
		uint8_t value_c[4];
	} value;

	if (buf[offset++] != slvaddr) {
		return -1;
	}

	function = buf[offset++];
	switch(function) {
		case 0x03: 
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
	char buf[256];

	while(state != MB_FINISH) {
		switch(state) {
			case MB_REQUEST:
				ret = rkGenModbusMsg03(buf, sizeof(buf), (uint8_t)edev->devid, (uint16_t)edev->regaddr, (uint8_t)edev->datatype);
#if 0
				printf("\033[40;31mCOM[%d] SEND[%d]:\033[0m\n  ", edev->com, edev->id);
				printHex(buf, ret);
#endif
				ret = rkSendModbusReq(edev->fd, buf, ret);
				if (ret < 0) {
					state = MB_ERROR;
					break;
				}
				state = MB_RECEIVE;
				break;
			case MB_RECEIVE:
				ret = rkRecvModbusResp(buf, (uint8_t)edev->devid, (uint16_t)edev->regaddr, edev->fd);
				if (ret < 0) {
					state = MB_ERROR;
					break;
				}
#if 0
				printf("\033[40;32mCOM[%d] RECV[%d]:\033[0m\n  ", edev->com, edev->id);
				printHex(buf, ret);
#endif
				state = MB_PROCESS;
				break;
			case MB_PROCESS:
				rkMbProcResp(buf, edev->devid, &edev->vals[RTD_RAW_VAL_OFFSET]);
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
