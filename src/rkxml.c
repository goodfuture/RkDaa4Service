/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.0
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkxml.c
 * Created At		: 2013-09-23 13:56
 * Last Modified	: 2013-09-27 11:49
 * Description		: 
 * History			: 
*******************************************************************************/

#include "rkxml.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

static char workDir[256];

void rkXmlSetWorkDir(const char *dir)
{
	memset(workDir, 0, sizeof(workDir));
	strncpy(workDir, dir, sizeof(workDir) - 1);
}

XmlNode *rkXmlNewXml()
{
	return mxmlNewXML("1.0");
}

XmlNode *rkXmlLoadFile(const char *fileName)
{
	char path[512];

	bzero(path, sizeof(path));
	strncpy(path, workDir, sizeof(path) - 1);
	if (path[strlen(path) - 1] != '/') {
		strcat(path, "/");
	}
	strcat(path, fileName);

	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		fprintf(stderr, "can't open file \'%s\' : %s.\n", path, strerror(errno));
		return NULL;
	}

	XmlNode *node = mxmlLoadFile(NULL, fp, rkXmlLoadCallBack);
	fclose(fp);

#if 0
	node = mxmlGetFirstChild(node); /* get root node */
	if (!node) {
		fprintf(stderr, "find error root node when parse file %s.\n", path);
		rkXmlFree(node);
		return NULL;
	}
#endif

	return node;
}

int rkXmlGetNodeDepth(XmlNode *node)
{
	int depth = 0;

	while(node) {
		node = mxmlGetParent(node);
		if (node) {
			depth++;
		}
	}

	return depth;
}

XmlType rkXmlLoadCallBack(XmlNode *node)
{
	const char *name = mxmlGetElement(node);

	if (!strcmp(name, "parameter")) {
		return MXML_TEXT;
	}

	return MXML_ELEMENT;
}

const char *rkXmlSaveCallBack(XmlNode *node, int where)
{
	static char space[16];
	int depth;
	int cnt, arg;

	if (where == MXML_WS_BEFORE_OPEN) {
		bzero(space, sizeof(space));
		depth = rkXmlGetNodeDepth(node);
		for (cnt = 0; cnt < depth - 1; cnt++) {
			space[cnt] = '\t';
		}
		return space;
	}

	if (where == MXML_WS_AFTER_OPEN && !mxmlGetText(node, &arg)) {
		return "\n";
	}

	if (where == MXML_WS_BEFORE_CLOSE && !mxmlGetText(node, &arg)) {
		bzero(space, sizeof(space));
		depth = rkXmlGetNodeDepth(node);
		for (cnt = 0; cnt < depth - 1; cnt++) {
			space[cnt] = '\t';
		}
		return space;
	}

	if (where == MXML_WS_AFTER_CLOSE) {
		return "\n";
	}

	return NULL;
}


int rkXmlSaveFile(const char *fileName, XmlNode *declareNode)
{
	int ret;
	char path[512];
	FILE *fp;

	bzero(path, sizeof(path));
	strncpy(path, workDir, sizeof(path) - 1);
	if (path[strlen(path) - 1] != '/') {
		strcat(path, "/");
	}
	strcat(path, fileName);

	fp = fopen(path, "w+");

	ret = mxmlSaveFile(declareNode, fp, rkXmlSaveCallBack);
	fclose(fp);

	return ret;
}

void rkXmlFree(XmlNode *node) 
{
	mxmlDelete(node);
}

int rkXmlSetParamVal(XmlNode *parent, const char *paramName, const char *paramVal)
{
	XmlNode *node = mxmlGetFirstChild(parent);

	for (; node; node = mxmlGetNextSibling(node)) {
		if (strcmp(mxmlGetElement(node), "parameter")) {
			fprintf(stderr, "can't parse node %s.\n", mxmlGetElement(node));
			continue;
		}

		const char *pname = mxmlElementGetAttr(node, "name");
		if (!strcmp(pname, paramName)) {
			XmlNode *child = mxmlGetFirstChild(node);
			if(mxmlGetType(child) == MXML_TEXT) {
				mxmlSetText(child, 0, paramVal);
			} else {
				mxmlNewText(node, 0, paramVal);
			}
			return 0;
		}
	}

	node =  mxmlNewElement(parent, "parameter");
	mxmlElementSetAttr(node, "name", paramName);
	mxmlNewText(node, 0, paramVal);

	return 0;
}

int rkXmlSetGroupParamVal(XmlNode *parent, const char *groupName, const char *paramName, const char *paramVal)
{
	XmlNode *group, *node, *child;

	group = mxmlGetFirstChild(parent);

	for (; group; group = mxmlGetNextSibling(group)) {
		if (mxmlGetType(group) != MXML_ELEMENT) {
			continue;
		}

		if (strcmp(mxmlGetElement(group), "group")) {
			continue;
		}

		if (strcmp(mxmlElementGetAttr(group, "id"), groupName)) {
			continue;
		}

		node = mxmlGetFirstChild(group);
		for (; node; node = mxmlGetNextSibling(node)) {
			if (mxmlGetType(node) != MXML_ELEMENT) {
				continue;
			}

			if (strcmp(mxmlGetElement(node), "parameter")){
				continue;
			}

			if (strcmp(mxmlElementGetAttr(node, "name"), paramName)) {
				continue;
			}

			child = mxmlGetFirstChild(node);
			if (mxmlGetType(child) == MXML_TEXT) {
				mxmlSetText(child, 0, paramVal);
			} else {
				mxmlNewText(node, 0, paramVal);
			}
			return 1;
		}

		node = mxmlNewElement(group, "parameter");
		mxmlElementSetAttr(node, "name", paramName);
		mxmlNewText(node, 0, paramVal);

		return 1;
	}

	group = mxmlNewElement(parent, "group");
	mxmlElementSetAttr(group, "id", groupName);
	node = mxmlNewElement(group, "parameter");
	mxmlElementSetAttr(node, "name", paramName);
	mxmlNewText(node, 0, paramVal);

	return 1;
}

int rkXmlSaveSysParam(const char *name, void *value)
{
	XmlNode *node, *head;
	char buf[128];

	head= rkXmlLoadFile(SYS_CFG_FILE);
	if (!head) {
		head = rkXmlNewXml();
	}

	node = mxmlGetFirstChild(head);
	if (!node) {
		node = mxmlNewElement(head, "system");
	}

	if (!strcmp(name, XML_SYSTEM_PARAM_NAME_SIM_NUM) || !strcmp(name, XML_SYSTEM_PARAM_NAME_MN_NUM) || !strcmp(name, XML_SYSTEM_PARAM_NAME_PASSWORD)) {
		rkXmlSetParamVal(node, name, (char *)value);
	} else if (!strcmp(name, XML_SYSTEM_PARAM_NAME_SYSTEM_TYPE)) {
		sprintf(buf, "%d", *(uint8_t *)value);
		rkXmlSetParamVal(node, name, buf);
	} else if (!strcmp(name, XML_SYSTEM_PARAM_NAME_DSI) || !strcmp(name, XML_SYSTEM_PARAM_NAME_DUI)) {
		sprintf(buf, "%d", *(uint16_t *)value);
		rkXmlSetParamVal(node, name, buf);
	} else if (!strcmp(name, XML_SYSTEM_PARAM_NAME_DUM)) {
		switch(*(DUM_T *)value) {
			case DTU:
				sprintf(buf, "DTU");
				break;
			case ETH:
				sprintf(buf, "ETH");
				break;
			case BOTH:
				sprintf(buf, "DTU+ETH");
				break;
			default:
				sprintf(buf, "DTU");
				break;
		}
		rkXmlSetParamVal(node, name, buf);
	} else if (!strcmp(name, XML_SYSTEM_PARAM_NAME_RTD_UPLOAD) || !strcmp(name, XML_SYSTEM_PARAM_NAME_MINUTES_DATA_UPLOAD) || !strcmp(name, XML_SYSTEM_PARAM_NAME_HOUR_DATA_UPLOAD) || !strcmp(name, XML_SYSTEM_PARAM_NAME_DAY_DATA_UPLOAD) || !strcmp(name, XML_SYSTEM_PARAM_NAME_SWITCH_CHANGE_UPLOAD) || !strcmp(name, XML_SYSTEM_PARAM_NAME_ALARM_UPLOAD)) {
		if (*(uint8_t *)value) {
			rkXmlSetParamVal(node, name, "EN");
		} else {
			rkXmlSetParamVal(node, name, "DIS");
		}
	}

	rkXmlSaveFile(SYS_CFG_FILE, head);
	rkXmlFree(head);

	return 1;
}

int rkXmlSaveAllSysParam(struct sys *sys)
{
	XmlNode *node, *head;
	char buf[128];

	head= rkXmlLoadFile(SYS_CFG_FILE);
	if (!head) {
		head = rkXmlNewXml();
	}

	node = mxmlGetFirstChild(head);
	if (!node) {
		node = mxmlNewElement(head, "system");
	}

	rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_SIM_NUM, sys->sim);
	rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_MN_NUM, sys->mn);
	rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_PASSWORD, sys->pw);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", sys->st);
	rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_SYSTEM_TYPE, buf);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", sys->dsi);
	rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_DSI, buf);

	if (sys->dum % 3 == 0) {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_DUM, "DTU");
	} else if (sys->dum % 3 == 1) {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_DUM, "ETH");
	} else if (sys->dum % 3 == 2) {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_DUM, "DTU+ETH");
	}

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", sys->dui);
	rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_DUI, buf);

	if (sys->rduen == 0) {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_RTD_UPLOAD, "DIS");
	} else {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_RTD_UPLOAD, "EN");
	}

	if (sys->mduen == 0) {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_MINUTES_DATA_UPLOAD, "DIS");
	} else {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_MINUTES_DATA_UPLOAD, "EN");
	}

	if (sys->hduen == 0) {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_HOUR_DATA_UPLOAD, "DIS");
	} else {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_HOUR_DATA_UPLOAD, "EN");
	}

	if (sys->dduen == 0) {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_DAY_DATA_UPLOAD, "DIS");
	} else {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_DAY_DATA_UPLOAD, "EN");
	}

	if (sys->sduen == 0) {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_SWITCH_CHANGE_UPLOAD, "DIS");
	} else {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_SWITCH_CHANGE_UPLOAD, "EN");
	}

	if (sys->alarmen == 0) {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_ALARM_UPLOAD, "DIS");
	} else {
		rkXmlSetParamVal(node, XML_SYSTEM_PARAM_NAME_ALARM_UPLOAD, "EN");
	}

	rkXmlSaveFile(SYS_CFG_FILE, head);
	rkXmlFree(head);

	return 1;
}

int rkXmlParseSysParam(struct sys *handle)
{
	int ret, arg;
	XmlNode *node, *head;

	head = rkXmlLoadFile(SYS_CFG_FILE);
	if (!head) {
		return 0;
	}

	node = mxmlGetFirstChild(head);

	for (node = mxmlGetFirstChild(node); node; node = mxmlGetNextSibling(node)) {
		if (strcmp(mxmlGetElement(node), "parameter")) {
			fprintf(stderr, "can't parse node %s.\n", mxmlGetElement(node));
			continue;
		}

		const char *param = mxmlElementGetAttr(node, "name");
		const char *val = mxmlGetText(node, &arg);
		if (!param || !val) {
			continue;
		}

		if (!strcmp(param, XML_SYSTEM_PARAM_NAME_SIM_NUM)) {
			strcpy(handle->sim, val);
			ret++;
		} else if (!strcmp(param, XML_SYSTEM_PARAM_NAME_MN_NUM)) {
			strcpy(handle->mn, val);
			ret++;
		} else if (!strcmp(param, XML_SYSTEM_PARAM_NAME_SYSTEM_TYPE)) { /* system type */
			handle->st = atoi(val);
			ret++;
		} else if (!strcmp(param, XML_SYSTEM_PARAM_NAME_PASSWORD)) { /* password */
			strcpy(handle->pw, val);
			ret++;
		} else if (!strcmp(param, XML_SYSTEM_PARAM_NAME_DSI)) { /* data store interval */
			handle->dsi = atoi(val);
			ret++;
		} else if (!strcmp(param, XML_SYSTEM_PARAM_NAME_DUM)) { /* data upload mode */
			if (!strcmp(val, "DTU")) {
				handle->dum = DTU;
				ret++;
			} else if (!strcmp(val, "ETH")) {
				handle->dum = ETH;
				ret++;
			} else if (!strcmp(val, "DTU+ETH")) {
				handle->dum = BOTH;
				ret++;
			}
		} else if (!strcmp(param, XML_SYSTEM_PARAM_NAME_DUI)) { /* data upload interval */
			handle->dui = atoi(val);
			ret++;
		} else if (!strcmp(param, XML_SYSTEM_PARAM_NAME_RTD_UPLOAD)) { /* real time data enable */
			handle->rduen= !strcmp(val, "EN") ? 1 : 0;
			ret++;
		} else if (!strcmp(param, XML_SYSTEM_PARAM_NAME_MINUTES_DATA_UPLOAD)) { /* minus data enable */
			handle->mduen = !strcmp(val, "EN") ? 1 : 0;
			ret++;
		} else if (!strcmp(param, XML_SYSTEM_PARAM_NAME_HOUR_DATA_UPLOAD)) { /* hour data enable */
			handle->hduen = !strcmp(val, "EN") ? 1 : 0;
			ret++;
		} else if (!strcmp(param, XML_SYSTEM_PARAM_NAME_DAY_DATA_UPLOAD)) { /* daily data enable */
			handle->dduen = !strcmp(val, "EN") ? 1 : 0;
			ret++;
		} else if (!strcmp(param, XML_SYSTEM_PARAM_NAME_SWITCH_CHANGE_UPLOAD)) { /* discrete data enable */
			handle->sduen = !strcmp(val, "EN") ? 1 : 0;
			ret++;
		} else if (!strcmp(param, XML_SYSTEM_PARAM_NAME_ALARM_UPLOAD)) { /* alarm enable */
			handle->alarmen = !strcmp(val, "EN") ? 1 : 0;
			ret++;
		}
	}
	rkXmlFree(head);

	return ret;
}

int rkXmlSaveNetParam(struct net *net) 
{
	XmlNode *node, *head;
	char buf[128];

	head= rkXmlLoadFile(NET_CFG_FILE);
	if (!head) {
		head = rkXmlNewXml();
	}

	node = mxmlGetFirstChild(head);
	if (!node) {
		node = mxmlNewElement(head, "network");
	}

	rkXmlSetParamVal(node, "LOCALADDR", net->laddr);
	rkXmlSetParamVal(node, "NETMASK", net->mask);
	rkXmlSetParamVal(node, "GATEWAY", net->gw);
	rkXmlSetParamVal(node, "DNS", net->dns);

	memset(buf, 0, sizeof(buf));
	if (net->cm == UDPCLIENT) {
		sprintf(buf, "UDPCLIENT");
	} else if (net->cm == UDPSERVER) {
		sprintf(buf, "UDPSERVER");
	} else if (net->cm == TCPCLIENT) {
		sprintf(buf, "TCPCLIENT");
	} else if (net->cm == TCPSERVER) {
		sprintf(buf, "TCPSERVER");
	}
	rkXmlSetParamVal(node, "CM", buf);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", net->lport);
	rkXmlSetParamVal(node, "LOCALPORT", buf);

	rkXmlSetParamVal(node, "REMOTEADDR", net->raddr);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%d", net->rport);
	rkXmlSetParamVal(node, "REMOTEPORT", buf);

	rkXmlSaveFile(NET_CFG_FILE, head);
	rkXmlFree(head);

	return 1;
}

int rkXmlParseNetParam(struct net *handle)
{
	int ret = 0, arg;
	XmlNode *node, *head;

	head = rkXmlLoadFile(NET_CFG_FILE);
	if (!head) {
		return 0;
	}

	node = mxmlGetFirstChild(head);

	for (node = mxmlGetFirstChild(node); node; node = mxmlGetNextSibling(node)) {
		if (strcmp(mxmlGetElement(node), "parameter")) {
			fprintf(stderr, "can't parse node %s.\n", mxmlGetElement(node));
			continue;
		}

		const char *param = mxmlElementGetAttr(node, "name");
		const char *val = mxmlGetText(node, &arg);
		if (!param || !val) {
			continue;
		}

		if (!strcmp(param, "LOCALADDR")) {
			strncpy(handle->laddr, val, sizeof(handle->laddr) - 1);
			ret++;
		} else if (!strcmp(param, "LOCALPORT")) {
			handle->lport = atoi(val);
			ret++;
		} else if (!strcmp(param, "NETMASK")) {
			strncpy(handle->mask , val, sizeof(handle->mask) - 1);
			ret++;
		} else if (!strcmp(param, "GATEWAY")) {
			strncpy(handle->gw , val, sizeof(handle->gw) - 1);
			ret++;
		} else if (!strcmp(param, "DNS")) {
			strncpy(handle->dns , val, sizeof(handle->dns) - 1);
			ret++;
		} else if (!strcmp(param, "CM")) {
			if (!strcmp(val, "UDPCLIENT")) {
				handle->cm = UDPCLIENT;
				ret++;
			} else if (!strcmp(val, "UDPSERVER")) {
				handle->cm = UDPSERVER;
				ret++;
			} else if (!strcmp(val, "TCPCLIENT")) {
				handle->cm = TCPCLIENT;
				ret++;
			} else if (!strcmp(val, "TCPSERVER")) {
				handle->cm = TCPSERVER;
				ret++;
			}
		} else if (!strcmp(param, "REMOTEADDR")) { 
			int index = 0, drop = 1, w_pos = 0;
			while(val[index] != '\0' && index < 16) {
				if (drop == 1 && val[index] == '0') {
					index++;
					continue;
				}

				drop = val[index] == '.' ? 1 : 0;

				handle->raddr[w_pos++] = val[index];
				index++;
			}
			ret++;
		} else if (!strcmp(param, "REMOTEPORT")) {
			handle->rport = atoi(val);
			ret++;
		}
	}
	rkXmlFree(head);

	return ret;
}

int rkXmlSaveUartParam(int channel, struct edev *param)
{
	XmlNode *node, *head;
	char buf[128], groupName[16];

	head= rkXmlLoadFile(SER_CFG_FILE);
	if (!head) {
		head = rkXmlNewXml();
	}

	node = mxmlGetFirstChild(head);
	if (!node) {
		node = mxmlNewElement(head, "uart");
	}

	sprintf(groupName, "CH%d", channel);

	if (param->inuse) {
		rkXmlSetGroupParamVal(node, groupName, "INUSE", "EN");
	} else {
		rkXmlSetGroupParamVal(node, groupName, "INUSE", "DIS");
	}

	rkXmlSetGroupParamVal(node, groupName, "CODE", param->code);

	sprintf(buf, "COM%d", param->com);
	rkXmlSetGroupParamVal(node, groupName, "COM", buf);

	if (param->datatype == INT) {
		rkXmlSetGroupParamVal(node, groupName, "DT", "INT");
	} else if (param->datatype == FLT) {
		rkXmlSetGroupParamVal(node, groupName, "DT", "FLT");
	}

	sprintf(buf, "%d", param->devid);
	rkXmlSetGroupParamVal(node, groupName, "DA", buf);

	sprintf(buf, "%d", param->regaddr);
	rkXmlSetGroupParamVal(node, groupName, "RA", buf);

	if (param->isconv) {
		rkXmlSetGroupParamVal(node, groupName, "ISCONV", "EN");
	} else {
		rkXmlSetGroupParamVal(node, groupName, "ISCONV", "DIS");
	}
	rkXmlSetGroupParamVal(node, groupName, "ISCONV", buf);

	if (param->usefml) {
		rkXmlSetGroupParamVal(node, groupName, "USEFML", "EN");
	} else {
		rkXmlSetGroupParamVal(node, groupName, "USEFML", "DIS");
	}

	rkXmlSetGroupParamVal(node, groupName, "FMLEXPR", param->fml);
	rkXmlSaveFile(SER_CFG_FILE, head);
	rkXmlFree(head);

	return 1;
}

int rkXmlSaveComParam(int channel, struct com *param)
{
	XmlNode *node, *head;
	char buf[128], groupName[16];

	head= rkXmlLoadFile(SER_CFG_FILE);
	if (!head) {
		head = rkXmlNewXml();
	}

	node = mxmlGetFirstChild(head);
	if (!node) {
		node = mxmlNewElement(head, "uart");
	}

	sprintf(groupName, "COM%d", channel);

	sprintf(buf, "%d", param->baud);
	rkXmlSetGroupParamVal(node, groupName, "BAUD", buf);

	sprintf(buf, "%d", param->db);
	rkXmlSetGroupParamVal(node, groupName, "DB", buf);

	sprintf(buf, "%d", param->sb);
	rkXmlSetGroupParamVal(node, groupName, "SB", buf);

	if (param->parity == NONE) {
		rkXmlSetGroupParamVal(node, groupName, "PARITY", "NONE");
	} else if (param->parity == ODD) {
		rkXmlSetGroupParamVal(node, groupName, "PARITY", "ODD");
	} else if (param->parity == EVEN) {
		rkXmlSetGroupParamVal(node, groupName, "PARITY", "EVEN");
	}

	rkXmlSetGroupParamVal(node, groupName, "PROTO", param->proto);

	rkXmlSaveFile(SER_CFG_FILE, head);
	rkXmlFree(head);

	return 1;
}

int rkXmlSaveAllUartParam(struct uart *uart)
{
	XmlNode *node, *head;
	char buf[128], groupName[16];
	int index;

	head= rkXmlLoadFile(SER_CFG_FILE);
	if (!head) {
		head = rkXmlNewXml();
	}

	node = mxmlGetFirstChild(head);
	if (!node) {
		node = mxmlNewElement(head, "uart");
	}

	for (index = 0; index < COM_NUM; index++) {
		struct com *com = &uart->m_tComParam[index];

		memset(groupName, 0, sizeof(groupName));
		sprintf(groupName, "COM%d", index);

		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%d", com->baud);
		rkXmlSetGroupParamVal(node, groupName, "BAUD", buf);

		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%d", com->db);
		rkXmlSetGroupParamVal(node, groupName, "DB", buf);

		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%d", com->sb);
		rkXmlSetGroupParamVal(node, groupName, "SB", buf);

		memset(buf, 0, sizeof(buf));
		if (com->parity == NONE) {
			sprintf(buf, "NONE");
		} else if (com->parity == ODD) {
			sprintf(buf, "ODD");
		} else if (com->parity == EVEN) {
			sprintf(buf, "EVEN");
		}
		rkXmlSetGroupParamVal(node, groupName, "PARITY", buf);

		rkXmlSetGroupParamVal(node, groupName, "PROTO", com->proto);
	}

	for (index = 0; index < EI_NUM; index++) {
		struct edev *ei = &uart->m_tChannelParam[index];

		memset(groupName, 0, sizeof(groupName));
		sprintf(groupName, "CH%d", index);

		memset(buf, 0, sizeof(buf));
		if (ei->inuse == 0) {
			sprintf(buf, "DIS");
		} else {
			sprintf(buf, "EN");
		}
		rkXmlSetGroupParamVal(node, groupName, "INUSE", buf);

		rkXmlSetGroupParamVal(node, groupName, "CODE", ei->code);

		memset(buf, 0, sizeof(buf));
		sprintf(buf, "COM%d", ei->com);
		rkXmlSetGroupParamVal(node, groupName, "COM", buf);

		memset(buf, 0, sizeof(buf));
		if (ei->datatype == INT) {
			sprintf(buf, "INT");
		} else if (ei->datatype == FLT) {
			sprintf(buf, "FLT");
		}
		rkXmlSetGroupParamVal(node, groupName, "DT", buf);

		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%d", ei->devid);
		rkXmlSetGroupParamVal(node, groupName, "DA", buf);

		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%d", ei->regaddr);
		rkXmlSetGroupParamVal(node, groupName, "RA", buf);

		memset(buf, 0, sizeof(buf));
		if (ei->isconv == 0) {
			sprintf(buf, "DIS");
		} else {
			sprintf(buf, "EN");
		}
		rkXmlSetGroupParamVal(node, groupName, "ISCONV", buf);

		memset(buf, 0, sizeof(buf));
		if (ei->usefml == 0) {
			sprintf(buf, "DIS");
		} else {
			sprintf(buf, "EN");
		}
		rkXmlSetGroupParamVal(node, groupName, "USEFML", buf);

		rkXmlSetGroupParamVal(node, groupName, "FMLEXPR", ei->fml);
	}

	rkXmlSaveFile(SER_CFG_FILE, head);
	rkXmlFree(head);

	return 1;
}

int rkXmlParseUartParam(struct uart *handle)
{
	int ret = 0;
	XmlNode *node, *head;

	head = rkXmlLoadFile(SER_CFG_FILE);
	if (!head) {
		//rkXmlSaveFile(SER_CFG_FILE, handle);
		return 0;
	}

	node = mxmlGetFirstChild(head);

	/* First Parameter */
	for(node = mxmlGetFirstChild(node); node; node = mxmlGetNextSibling(node)) {
		if (strcmp(mxmlGetElement(node), "group")) {
			fprintf(stderr, "can't parse node %s.\n", mxmlGetElement(node));
			continue;
		}

		const char *id = mxmlElementGetAttr(node, "id");
		int chl, iscom, arg;
		if (!strncmp(id, "COM", 3)) {
			iscom = 1;
		} else if (!strncmp(id, "CH", 2)) {
			iscom = 0;
		}
		chl = !iscom ? atoi(id + 2) : atoi(id + 3);

		XmlNode *child;
		const char *param;
		const char *val;
		if (iscom) {
			struct com *com = &handle->m_tComParam[chl];
			for (child = mxmlGetFirstChild(node); child; child = mxmlGetNextSibling(child)) {
				param = mxmlElementGetAttr(child, "name");
				val = mxmlGetText(child, &arg);

				if (!param || !val) {
					continue;
				}

				if (!strcmp(param, "PROTO")) {
					bzero(com->proto, sizeof(com->proto));
					strncpy(com->proto, val, sizeof(com->proto) - 1);
					ret++;
				}else if (!strcmp(param, "BAUD")) {
					com->baud = atoi(val);
					ret++;
				} else if (!strcmp(param, "DB")) {
					com->db = atoi(val);
					ret++;
				} else if (!strcmp(param, "SB")) {
					com->sb = atoi(val);
					ret++;
				} else if (!strcmp(param, "PARITY")) {
					if (!strcmp(val, "ODD")) {
						com->parity = ODD;
						ret++;
					} else if (!strcmp(val, "EVEN")) {
						com->parity = EVEN;
						ret++;
					} else if (!strcmp(val, "NONE")) {
						com->parity = NONE;
						ret++;
					}
				}
			}
		} else {
			struct edev *ei = &handle->m_tChannelParam[chl];
			ei->id = chl;

			for (child = mxmlGetFirstChild(node); child; child = mxmlGetNextSibling(child)) {
				param = mxmlElementGetAttr(child, "name");
				val = mxmlGetText(child, &arg);
				if (!param || !val) {
					continue;
				}

				if (!strcmp(param, "INUSE")) {
					ei->inuse = !strcmp(val, "EN") ? 1 : 0;
					ret++;
				} else if (!strcmp(param, "CODE")) {
					strncpy(ei->code, val, sizeof(ei->code) - 1);
					ret++;
				} else if (!strcmp(param, "COM")) {
					ei->com = atoi(&val[3]);
					ret++;
				} else if (!strcmp(param, "DA")) {
					ei->devid = atoi(val);
					ret++;
				} else if (!strcmp(param, "RA")) {
					ei->regaddr = atoi(val) - 1;
					ret++;
				} else if (!strcmp(param, "DT")) {
					ei->datatype = !strcmp(val, "FLT") ? FLT : INT;
					ret++;
				} else if (!strcmp(param, "ISCONV")) {
					ei->isconv = !strcmp(val, "EN") ? 1 : 0;
					ret++;
				} else if (!strcmp(param, "USEFML")) {
					ei->usefml = !strcmp(val, "EN") ? 1 : 0;
					ret++;
				} else if (!strcmp(param, "FMLEXPR")) {
					strncpy(ei->fml, val, sizeof(ei->fml) - 1);
					ret++;
				}
			}
		}
	}
	rkXmlFree(head);

	return ret;
}

int rkXmlSaveDioParam(struct dio *dio)
{
	XmlNode *node, *head;
	char groupName[16];
	int index;

	head= rkXmlLoadFile(DIO_CFG_FILE);
	if (!head) {
		head = rkXmlNewXml();
	}

	node = mxmlGetFirstChild(head);
	if (!node) {
		node = mxmlNewElement(head, "dio");
	}

	for	(index = 0; index < DI_NUM; index++) {
		memset(groupName, 0, sizeof(groupName));
		sprintf(groupName, "DI%d", index);

		struct dioparam *dip = &dio->m_tDiParam[index];

		if (dip->inuse == 0) {
			rkXmlSetGroupParamVal(node, groupName, "INUSE", "DIS");
		} else {
			rkXmlSetGroupParamVal(node, groupName, "INUSE", "EN");
		}

		rkXmlSetGroupParamVal(node, groupName, "CODE", dip->code);
	}

	for (index = 0; index < DO_NUM; index++) {
		memset(groupName, 0, sizeof(groupName));
		sprintf(groupName, "DO%d", index);

		struct dioparam *dop = &dio->m_tDoParam[index];

		if (dop->inuse == 0) {
			rkXmlSetGroupParamVal(node, groupName, "INUSE", "DIS");
		} else {
			rkXmlSetGroupParamVal(node, groupName, "INUSE", "EN");
		}

		rkXmlSetGroupParamVal(node, groupName, "CODE", dop->code);

		if (dop->val == 0) {
			rkXmlSetGroupParamVal(node, groupName, "STATE", "OFF");
		} else {
			rkXmlSetGroupParamVal(node, groupName, "STATE", "ON");
		}
	}

	rkXmlSaveFile(DIO_CFG_FILE, head);
	rkXmlFree(head);

	return 1;
}

int rkXmlParseDioParam(struct dio *handle)
{
	int ret = 0;
	XmlNode *node, *head;

	head = rkXmlLoadFile(DIO_CFG_FILE);
	if (!head) {
		return 0;
	}

	node = mxmlGetFirstChild(head);

	/* First Parameter */
	for(node = mxmlGetFirstChild(node); node; node = mxmlGetNextSibling(node)) {
		if (strcmp(mxmlGetElement(node), "group")) {
			fprintf(stderr, "can't parse node %s.\n", mxmlGetElement(node));
			continue;
		}

		const char *id = mxmlElementGetAttr(node, "id");
		int chl, isdi, arg;
		if (!strncmp(id, "DI", 2)) {
			isdi = 1;
		} else if (!strncmp(id, "DO", 2)) {
			isdi = 0;
		}
		chl = atoi(&id[2]);

		XmlNode *child;
		for (child = mxmlGetFirstChild(node); child; child = mxmlGetNextSibling(child)) {
			const char *param = mxmlElementGetAttr(child, "name");
			const char *val = mxmlGetText(child, &arg);
			if (!param || !val) {
				continue;
			}

			if (isdi) {
				struct dioparam *dip = &handle->m_tDiParam[chl];
				dip->id = chl;
				if (!strcmp(param, "INUSE")) {
					dip->inuse = !strcmp(val, "EN") ? 1 : 0;
					ret++;
				} else if (!strcmp(param, "CODE")) {
					strncpy(dip->code, val, sizeof(dip->code) - 1);
					ret++;
				}
			} else {
				struct dioparam *dop = &handle->m_tDoParam[chl];
				dop->id = chl;
				if (!strcmp(param, "INUSE")) {
					dop->inuse = !strcmp(val, "EN") ? 1 : 0;
					ret++;
				} else if (!strcmp(param, "CODE")) {
					strncpy(dop->code, val, sizeof(dop->code) - 1);
					ret++;
				} else if (!strcmp(param, "STATE")) {
					dop->val = !strcmp(val, "ON") ? ON : OFF;
					ret++;
				}
			}
		}
	}
	rkXmlFree(head);

	return ret;
}

int rkXmlSaveAnalogParam(int channel, struct aiparam *param)
{
	XmlNode *node, *head;
	char buf[128], groupName[16];

	head= rkXmlLoadFile(AI_CFG_FILE);
	if (!head) {
		head = rkXmlNewXml();
	}

	node = mxmlGetFirstChild(head);
	if (!node) {
		node = mxmlNewElement(head, "analog");
	}

	sprintf(groupName, "CH%d", channel);

	if (param->inuse) {
		rkXmlSetGroupParamVal(node, groupName, "INUSE", "EN");
	} else {
		rkXmlSetGroupParamVal(node, groupName, "INUSE", "DIS");
	}

	rkXmlSetGroupParamVal(node, groupName, "CODE", param->code);

	switch(param->type) {
		case MA20:
			rkXmlSetGroupParamVal(node, groupName, "TYPE", "4-20MA");
			break;
		case V5:
			rkXmlSetGroupParamVal(node, groupName, "TYPE", "0-5V");
			break;
		default:
			rkXmlSetGroupParamVal(node, groupName, "TYPE", "4-20MA");
			break;
	}

	sprintf(buf, "%f", param->ulv);
	rkXmlSetGroupParamVal(node, groupName, "ULV", buf);

	sprintf(buf, "%f", param->llv);
	rkXmlSetGroupParamVal(node, groupName, "LLV", buf);

	sprintf(buf, "%f", param->utv);
	rkXmlSetGroupParamVal(node, groupName, "UTV", buf);

	sprintf(buf, "%f", param->ltv);
	rkXmlSetGroupParamVal(node, groupName, "LTV", buf);

	if (param->isconv) {
		rkXmlSetGroupParamVal(node, groupName, "ISCONV", "EN");
	} else {
		rkXmlSetGroupParamVal(node, groupName, "ISCONV", "DIS");
	}

	if (param->usefml) {
		rkXmlSetGroupParamVal(node, groupName, "USEFML", "EN");
	} else {
		rkXmlSetGroupParamVal(node, groupName, "USEFML", "DIS");
	}

	rkXmlSetGroupParamVal(node, groupName, "FMLEXPR", param->fml);

	rkXmlSaveFile(AI_CFG_FILE, head);
	rkXmlFree(head);

	return 1;
}

int rkXmlSaveAllAnalogParam(struct analog *analog)
{
	XmlNode *node, *head;
	char buf[128], groupName[16];
	int index;

	head= rkXmlLoadFile(AI_CFG_FILE);
	if (!head) {
		head = rkXmlNewXml();
	}

	node = mxmlGetFirstChild(head);
	if (!node) {
		node = mxmlNewElement(head, "analog");
	}

	for	(index = 0; index < AI_NUM; index++) {
		struct aiparam *ai = &analog->m_tChannelParam[index];

		memset(groupName, 0, sizeof(groupName));
		sprintf(groupName, "CH%d", index);

		memset(buf, 0, sizeof(buf));
		if (ai->inuse == 0) {
			sprintf(buf, "DIS");
		} else {
			sprintf(buf, "EN");
		}
		rkXmlSetGroupParamVal(node, groupName, "INUSE", buf);
		rkXmlSetGroupParamVal(node, groupName, "CODE", ai->code);

		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%f", ai->ulv);
		rkXmlSetGroupParamVal(node, groupName, "ULV", buf);

		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%f", ai->llv);
		rkXmlSetGroupParamVal(node, groupName, "LLV", buf);

		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%f", ai->utv);
		rkXmlSetGroupParamVal(node, groupName, "UTV", buf);

		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%f", ai->ltv);
		rkXmlSetGroupParamVal(node, groupName, "LTV", buf);

		memset(buf, 0, sizeof(buf));
		if (ai->isconv == 0) {
			sprintf(buf, "DIS");
		} else {
			sprintf(buf, "EN");
		}
		rkXmlSetGroupParamVal(node, groupName, "ISCONV", buf);

		memset(buf, 0, sizeof(buf));
		if (ai->usefml == 0) {
			sprintf(buf, "DIS");
		} else {
			sprintf(buf, "EN");
		}
		rkXmlSetGroupParamVal(node, groupName, "USEFML", buf);

		rkXmlSetGroupParamVal(node, groupName, "FMLEXPR", ai->fml);
	}

	rkXmlSaveFile(AI_CFG_FILE, head);
	rkXmlFree(head);

	return 1;
}

int rkXmlParseAnalogParam(struct analog *analog)
{
	int ret = 0, arg;
	XmlNode *node, *head;

	head = rkXmlLoadFile(AI_CFG_FILE);
	if (!head) {
		return 0;
	}

	node = mxmlGetFirstChild(head);

	/* first parameter */
	for(node = mxmlGetFirstChild(node); node; node = mxmlGetNextSibling(node)) {
		if (strcmp(mxmlGetElement(node), "group")) {
			fprintf(stderr, "can't parse node %s.\n", mxmlGetElement(node));
			continue;
		}

		const char *id = mxmlElementGetAttr(node, "id");
		int chl = atoi(&id[2]);
		struct aiparam *ai = &analog->m_tChannelParam[chl];
		XmlNode *child;
		for(child = mxmlGetFirstChild(node); child; child = mxmlGetNextSibling(child)) {
			const char *param = mxmlElementGetAttr(child, "name");
			const char *val = mxmlGetText(child, &arg);
			if (!param || !val) {
				continue;
			}

			ai->id = chl;
			if (!strcmp(param, "INUSE")) {
				ai->inuse = !strcmp(val, "EN") ? 1 : 0;
			} else if (!strcmp(param, "TYPE")) {
				if (!strcmp(val, "4-20MA")) {
					ai->type = MA20;
					ret++;
				} else if (!strcmp(val, "0-5V")) {
					ai->type = V5;
					ret++;
				}
			} else if (!strcmp(param, "CODE")) {
				strncpy(ai->code, val, sizeof(ai->code) - 1);
				ret++;
			} else if (!strcmp(param, "ULV")) {
				ai->ulv = atof(val);
				ret++;
			} else if (!strcmp(param, "LLV")) {
				ai->llv = atof(val);
				ret++;
			} else if (!strcmp(param, "UTV")) {
				ai->utv = atof(val);
				ret++;
			} else if (!strcmp(param, "LTV")) {
				ai->ltv = atof(val);
				ret++;
			} else if (!strcmp(param, "ISCONV")) {
				ai->isconv = !strcmp(val, "EN") ? 1 : 0;
				ret++;
			} else if (!strcmp(param, "USEFML")) {
				ai->usefml = !strcmp(val, "EN") ? 1 : 0;
				ret++;
			} else if (!strcmp(param, "FMLEXPR")) {
				strncpy(ai->fml, val, sizeof(ai->fml) - 1);
				ret++;
			}
		}

		if (ai->ulv < ai->llv) {
			ai->ulv = ai->llv;
		}

		if (ai->utv < ai->ltv) {
			ai->utv = ai->ltv;
		}

		if (ai->utv > ai->ulv) {
			ai->utv = ai->utv;
		}

		if (ai->ltv < ai->llv) {
			ai->ltv = ai->llv;
		}
	}
	rkXmlFree(head);

	return ret;
}

int rkXmlParseAiCalParam(struct analog *analog)
{
	int ret = 0, arg;
	XmlNode *node, *head;

	head = rkXmlLoadFile(AI_CALIB_FILE);
	if (!head) {
		rkXmlGenAiCalibFile(analog->m_tCalibrateParam, AI_NUM);
		return 0;
	}

	node = mxmlGetFirstChild(head);

	for (node = mxmlGetFirstChild(node); node; node = mxmlGetNextSibling(node)) {
		if (strcmp(mxmlGetElement(node), "group")) {
			fprintf(stderr, "can't parse node %s.\n", mxmlGetElement(node));
			continue;
		}

		const char *id = mxmlElementGetAttr(node, "id");
		int chl = atoi(&id[2]);
		XmlNode *child;
		for (child = mxmlGetFirstChild(node); child; child = mxmlGetNextSibling(child)) {
			if (strcmp(mxmlGetElement(child), "parameter")) {
				fprintf(stderr, "can't parse child %s.\n", mxmlGetElement(child));
				continue;
			}

			const char *param = mxmlElementGetAttr(child, "name");
			const char *val = mxmlGetText(child, &arg);
			if (!param || !val) {
				continue;
			}

			if (!strcmp(param, "LOFFSET")) {
				sscanf(val, "0x%04hX", &analog->m_tCalibrateParam[chl].loffset);
				ret++;
			} else if (!strcmp(param, "HOFFSET")) {
				sscanf(val, "0x%04hX", &analog->m_tCalibrateParam[chl].hoffset);
				ret++;
			}
		}
	}
	rkXmlFree(head);

	return ret;
}

int rkXmlGenAiCalibFile(struct aical *cal, uint8_t num)
{
	int i;
	FILE *fp;
	char buf[64];
	XmlNode *xml, *root, *node, *group;

	xml = mxmlNewXML("1.0");
	root = mxmlNewElement(xml, "AICalib");
	for (i = 0; i < num; i++) {
		sprintf(buf, "CH%d", i);
		group = mxmlNewElement(root, "group");
		mxmlElementSetAttr(group, "id", buf);
		node = mxmlNewElement(group, "parameter");
		mxmlElementSetAttr(node, "name", "LOFFSET");
		sprintf(buf, "0x%04hX", cal[i].loffset);
		mxmlNewText(node, 0, buf);
		node = mxmlNewElement(group, "parameter");
		mxmlElementSetAttr(node, "name", "HOFFSET");
		sprintf(buf, "0x%04hX", cal[i].hoffset);
		mxmlNewText(node, 0, buf);
	}

	sprintf(buf, "%s/aicalib.xml", DEF_CFG_FILE_DIR);
	fp = fopen(buf, "w+");
	if (!fp) {
		fprintf(stderr, "%s : failed to create file '%s' : %s.\n", __func__, buf, strerror(errno));
		return -1;
	}

	mxmlSetWrapMargin(120);
	if (mxmlSaveFile(xml, fp, rkXmlSaveCallBack) == -1) {
		fclose(fp);
		fprintf(stderr, "%s : failed to save file '%s' : %s.\n", __func__, buf, strerror(errno));
	}
	fclose(fp);
	mxmlDelete(xml);

	return 0;
}
