#ifndef _RK_MODBUS_H_
#define _RK_MODBUS_H_

#include "rktype.h"

#define MB_TIME_OUT_MS 1000
#define MB_ADU_MAX_LEN 253

typedef enum {
	MB_REQUEST = 0,
	MB_RECEIVE,
	MB_PROCESS,
	MB_FINISH,
	MB_ERROR,
} MB_STEP_T;

typedef struct mbRtuReqMsg {
	uint8_t addr;
	uint8_t func;
	uint16_t reg;
	uint16_t num;
	uint16_t crc;
} mbRtuReqMsg_t;

int init(void *handle);
char *name(void);
int run(void *handle);

#endif /* _RK_MODBUS_H_ */
