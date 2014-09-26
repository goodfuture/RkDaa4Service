#ifndef _RK_DAM_H_ /* DAM : Data Acquisition Module */
#define _RK_DAM_H_

#include "rktype.h"
#include "rkshm.h"
#include "rkfml.h"

int rkDamInit(struct context *ctx);
void rkDamRun(void *handle);
void rkDamIoThread(void *handle);
void rkDamEiThread(void *handle);
void rkDamEiChildThread(void *handle);
int rkDamGetAiVal(struct analog *analog);
int rkDamGetDiVal(struct dio *handle);
int rkDamDiRead(uint16_t *val);
int rkDamSetDoVal(struct dioparam *handle);
int rkDamPushAiInfo(struct analog *analog);
int rkDamPushEiInfo(struct edev *ei);
int rkDamPushDiInfo(struct dioparam *dip);
int rkDamPushDoInfo(struct dioparam *dop);
int rkDamAiDataSample(struct aiparam *ai);
int rkDamAiDataProc(struct aiparam *ai, struct aical *cal);
int rkDamEiDataProc(struct edev *ei);
int rkDamCalcStat(const char *code, struct statinfo *stat, float val);
int rkDamClrStatData(struct context *ctx, STAT_DATA_TYPE type);

#endif /* _RK_DAM_H_ */
