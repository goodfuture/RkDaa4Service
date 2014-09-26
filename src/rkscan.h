#ifndef _RK_SCAN_LIB_H_
#define _RK_SCAN_LIB_H_

#include "rktype.h"

#define MAX_LIB_NUM 10

typedef struct libMap {
	char *lname[MAX_LIB_NUM]; /* Lib Name */
	char *fpath[MAX_LIB_NUM]; /* Path */
	uint8_t size; /* Current Lib Number */
} libMap_t;

int rkLibScan(const char *path);
const char *rkLibGetPath(const char *lname);

#endif /* _RK_SCAN_LIB_H_ */
