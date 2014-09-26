#ifndef _RK_DEBUG_H_
#define _RK_DEBUG_H_

#include "rktype.h"

void printsys(struct sys sys);
void printnet(struct net net);
void printuart(struct uart uart, int flag);
void printdio(struct dio dio);
void printai(struct analog analog);

#endif /* _RK_DEBUG_H_ */
