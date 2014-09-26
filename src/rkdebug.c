/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.0
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkdebug.c
 * Created At		: 2013-09-23 14:28
 * Last Modified	: 2013-09-30 11:27
 * Description		: 
 * History			: 
*******************************************************************************/

#include "rkdebug.h"
#include <stdio.h>

void printsys(struct sys sys)
{
	printf(
			"sys.sim = %s.\n"
			"sys.mn = %s.\n"
			"sys.pw = %s.\n"
			"sys.st = %d.\n"
			"sys.dui = %d.\n"
			"sys.dum = %d.\n"
			"sys.dsi = %d.\n"
			"sys.rduen = %d.\n"
			"sys.mduen = %d.\n"
			"sys.hduen = %d.\n"
			"sys.dduen = %d.\n"
			"sys.sduen= %d.\n"
			"sys.alarmen = %d.\n",
			sys.sim,
			sys.mn,
			sys.pw,
			sys.st,
			sys.dui,
			sys.dum,
			sys.dsi,
			sys.rduen,
			sys.mduen,
			sys.hduen,
			sys.dduen,
			sys.sduen,
			sys.alarmen
				);
	return;
}

void printnet(struct net net)
{
	printf(
			"net.laddr = %s.\n"
			"net.lport = %d.\n"
			"net.mask = %s.\n"
			"net.gw = %s.\n"
			"net.dns = %s.\n"
			"net.raddr = %s.\n"
			"net.rport = %d.\n"
			"net.cm = %d.\n",
			net.laddr,
			net.lport,
			net.mask,
			net.gw,
			net.dns,
			net.raddr,
			net.rport,
			net.cm
		  );

	return;
}

void printuart(struct uart uart, int flag)
{
	int i;

	for (i = 0; i < COM_NUM; i++) {
		printf(
				"uart.m_tComParam[%d].id= %d.\n"
				"uart.m_tComParam[%d].proto = %s.\n"
				"uart.m_tComParam[%d].baud = %d.\n"
				"uart.m_tComParam[%d].db = %d.\n"
				"uart.m_tComParam[%d].sb = %d.\n"
				"uart.m_tComParam[%d].parity = %d.\n",
				i, uart.m_tComParam[i].id, 
				i, uart.m_tComParam[i].proto, 
				i, uart.m_tComParam[i].baud, 
				i, uart.m_tComParam[i].db, 
				i, uart.m_tComParam[i].sb, 
				i, uart.m_tComParam[i].parity
			  );
	}

	for (i = 0; i < EI_NUM; i++) {
		if ((flag && uart.m_tChannelParam[i].inuse) || !flag) {
			printf(
					"uart.m_tChannelParam[%d].id = %d.\n"
					"uart.m_tChannelParam[%d].inuse = %d.\n"
					"uart.m_tChannelParam[%d].code = %s.\n"
					"uart.m_tChannelParam[%d].com = %d.\n"
					"uart.m_tChannelParam[%d].devaddr = %d.\n"
					"uart.m_tChannelParam[%d].da = %d.\n"
					"uart.m_tChannelParam[%d].dt = %d.\n"
					"uart.m_tChannelParam[%d].isconv = %d.\n"
					"uart.m_tChannelParam[%d].usefml = %d.\n"
					"uart.m_tChannelParam[%d].formula = %s.\n",
					i, uart.m_tChannelParam[i].id, 
					i, uart.m_tChannelParam[i].inuse, 
					i, uart.m_tChannelParam[i].code, 
					i, uart.m_tChannelParam[i].com, 
					i, uart.m_tChannelParam[i].devid, 
					i, uart.m_tChannelParam[i].regaddr, 
					i, uart.m_tChannelParam[i].datatype, 
					i, uart.m_tChannelParam[i].isconv, 
					i, uart.m_tChannelParam[i].usefml, 
					i, uart.m_tChannelParam[i].fml
						);
		}
	}

	return;
}

void printdio(struct dio dio)
{
	int i;

	for (i = 0; i < DI_NUM; i++) {
		printf(
				"dio.m_tDiParam[%d].id = %d.\n"
				"dio.m_tDiParam[%d].inuse = %d.\n"
				"dio.m_tDiParam[%d].code = %s.\n",
				i, dio.m_tDiParam[i].id,
				i, dio.m_tDiParam[i].inuse,
				i, dio.m_tDiParam[i].code
			  );
	}

	for (i = 0; i < DO_NUM; i++) {
		printf(
				"dio.m_tDoParam[%d].id = %d.\n"
				"dio.m_tDoParam[%d].inuse = %d.\n"
				"dio.m_tDoParam[%d].code = %s.\n"
				"dio.m_tDoParam[%d].val = %d.\n",
				i, dio.m_tDoParam[i].id,
				i, dio.m_tDoParam[i].inuse,
				i, dio.m_tDoParam[i].code,
				i, dio.m_tDoParam[i].val
			  );
	}

	return;
}

void printai(struct analog analog)
{
	int i;

	for (i = 0; i < AI_NUM; i++) {
		printf(
				"analog.m_tChannelParam[%d].inuse = %d.\n"
				"analog.m_tChannelParam[%d].id = %d.\n"
				"analog.m_tChannelParam[%d].type = %d.\n"
				"analog.m_tChannelParam[%d].code = %s.\n"
				"analog.m_tChannelParam[%d].ulv = %f.\n"
				"analog.m_tChannelParam[%d].llv = %f.\n"
				"analog.m_tChannelParam[%d].utv = %f.\n"
				"analog.m_tChannelParam[%d].ltv = %f.\n"
				"analog.m_tChannelParam[%d].isconv = %d.\n"
				"analog.m_tChannelParam[%d].usefml = %d.\n"
				"analog.m_tChannelParam[%d].formula= %s.\n", 
				i, analog.m_tChannelParam[i].inuse,
				i, analog.m_tChannelParam[i].id,
				i, analog.m_tChannelParam[i].type, 
				i, analog.m_tChannelParam[i].code,
				i, analog.m_tChannelParam[i].ulv,
				i, analog.m_tChannelParam[i].llv,
				i, analog.m_tChannelParam[i].utv,
				i, analog.m_tChannelParam[i].ltv,
				i, analog.m_tChannelParam[i].isconv, 
				i, analog.m_tChannelParam[i].usefml,
				i, analog.m_tChannelParam[i].fml
					);
	} 

	return;
}
