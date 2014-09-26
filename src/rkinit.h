#ifndef _RK_INIT_H_
#define _RK_INIT_H_

#include "rktype.h"
#include "rkxml.h"
#include "rkserver.h"

int rkSysInit(struct context *ctx, int argc, char *argv[]);
int rkReloadUsrCfg(struct context *ctx);
void rkInitConfigParam(struct context *ctx);
void rkInitSysParam(struct sys *sys);
void rkInitNetParam(struct net *net);
void rkInitUartParam(struct uart *uart);
void rkInitDioParam(struct dio *dio);
void rkInitAnalogParam(struct analog *analog);
void rkInitAiCalParam(struct analog *analog);
int rkScanDynamicProtocol(struct context *ctx);

#endif /* _RK_INIT_H_ */
