/*******************************************************************************
 * Copyright(C)	    : 2013 Rockontrol Industrial Park, Inc.
 * Version          : V1.0
 * Author           : KangQi
 * Tel              : 18636181581/6581
 * Email            : goodfuturemail@gmail.com
 * File Name        : rkhmi.c
 * Created At       : 2013-09-25 14:08
 * Last Modified    : 2013-11-15 10:38
 * Description      : 
 * History	        : 
*******************************************************************************/

#include "rktype.h"
#include "rkinit.h"
#include "rkshm.h"
#include "rkhmi.h"
#include "rkdtm.h"
#include "unistd.h"

int rkHmiInit()
{
	int rc;

	rc = rkShmInit();

	return rc;
}

void rkHmiRun(void *handle)
{
	struct context *ctx = (struct context*)handle;
	static uint8_t	dtu_current_status = 1;
	uint8_t ret;

	while(1) {
		rkCoreBeatOnce(CORE_VERSION);
		sprintf(ctx->m_aVersion[0], "%s", rkGetGuiVersion());
		//printf("HMI : %s\n", ctx->m_aVersion[0]);

		if (rkIsReloadFlagBeenSet()) {
			rkReloadUsrCfg(ctx);
			rkClearReloadFlag();
		}

		if (IS_MSG_UPLOAD_BY_DTU(ctx->m_tSystemParam.dum) || IS_MSG_UPLOAD_BY_ETH_AND_DTU(ctx->m_tSystemParam.dum)) {
			ret = rkIsDtuDisabled();
			if (ret != 0 && dtu_current_status == 1) {
				rkDtmCloseDtu();
				dtu_current_status = 0;
			} else if (ret == 0 && dtu_current_status == 0) {
				if (rkDtmReuseDtu() == 0) {
					dtu_current_status = 1;
				}
			}
		}

		if (rkIsReqExit()) {
			rkClearExitReq();
			ctx->exit = 1;
		}
		usleep(100000);
	}
}
