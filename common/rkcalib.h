#ifndef _RK_CALIB_H_
#define _RK_CALIB_H_

#include "rkcom.h"
#include <stdint.h>

struct SCalib {
	uint8_t inuse;
	uint16_t loffset;
	uint16_t hoffset;
};

#ifdef __cplusplus
extern "C" {
#endif //! __cplusplus 

int rkCalibInit();
int rkCliCalib();
int rkCalibExec(int id, int type);
int rkGenCalibFile(const char *path);

#ifdef __cplusplus
}
#endif //! __cplusplus

#endif /* _RK_CALIB_H_ */
