#ifndef _RK_COU_H_
#define _RK_COU_H_

#include "rktype.h"

int rkCouGetCTblIndex(const char *scode);
int rkCouUpdateCTbl(const char *code, float val);
int rkCouInitCTbl(struct codetbl *ctbl);
int rkCouCalcStat(const char *code, struct statinfo *stat, float val);

#endif /* _RK_COU_H_ */
