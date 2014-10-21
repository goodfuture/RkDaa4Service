#ifndef _RK_SMC_H_
#define _RK_SMC_H_

#include "rktype.h"

#define SMC_TIME_OUT_MS 1000

typedef enum {
	SMC_REQUEST = 0,
	SMC_RECEIVE,
	SMC_FINISH,
	SMC_ERROR,
} SMC_STEP_T;

int init(void *handle);
char *name(void);
int run(void *handle);

#endif /* _RK_SMC_H_ */
