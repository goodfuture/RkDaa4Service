/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.0
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkcalib.c
 * Created At		: 2013-09-23 13:37
 * Last Modified	: 2013-12-03 14:40
 * Description		: 
 * History			: 
*******************************************************************************/
#include "rktype.h"
#include "rkcalib.h"
#include <mxml.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static struct SCalib s_aCalib[AI_NUM];

static int rkCalibXmlGetNodeDepth(mxml_node_t *node)
{
	int depth = 0;

	while(node) {
		node = mxmlGetParent(node);
		depth = !node ? depth : depth + 1;
	}

	return depth;
}

static mxml_type_t rkCalibXmlLoadCallBack(mxml_node_t *node)
{
	const char *name = mxmlGetElement(node);

	if (!strcmp(name, "parameter")) {
		return MXML_TEXT;
	}   

	return MXML_ELEMENT;
}

static const char *rkCalibXmlSaveCallBack(mxml_node_t *node, int where)
{
	static char space[16];
	int depth;
	int i;

	if (where == MXML_WS_BEFORE_OPEN) {
		bzero(space, sizeof(space));
		depth = rkCalibXmlGetNodeDepth(node);
		for (i = 0; i < depth - 1; i++) {
			space[i] = '\t';
		}   
		return space;
	}   

	if (where == MXML_WS_AFTER_OPEN && !mxmlGetText(node, 0)) {
		return "\n";
	}   

	if (where == MXML_WS_BEFORE_CLOSE && !mxmlGetText(node, 0)) {
		bzero(space, sizeof(space));
		depth = rkCalibXmlGetNodeDepth(node);
		for (i = 0; i < depth - 1; i++) {
			space[i] = '\t';
		}   
		return space;
	}   

	if (where == MXML_WS_AFTER_CLOSE) {
		return "\n";
	}   

	return NULL;
}

int rkCalibInit()
{
	int id;
	FILE *fp;
	char path[64];
	mxml_node_t *xml, *calib, *node, *child;

	memset(s_aCalib, 0, sizeof(struct SCalib) * AI_NUM);

	sprintf(path, "%s/%s", DEF_CFG_FILE_DIR, AI_CALIB_FILE);

	fp = fopen(path, "r");
	if (fp == NULL) {
		return 0;
	}

	xml = mxmlLoadFile(NULL, fp, rkCalibXmlLoadCallBack);
	calib = mxmlGetFirstChild(xml);
	fclose(fp);

	for(node = mxmlGetFirstChild(calib); node; node = mxmlGetNextSibling(node)) {
		if (strcmp(mxmlGetElement(node), "group")) {
			continue;
		}

		const char *ptr = mxmlElementGetAttr(node, "id");
		if (ptr == NULL) {
			continue;
		}

		id = atoi(&ptr[2]);

		s_aCalib[id].inuse = 1;

		for (child = mxmlGetFirstChild(node); child; child = mxmlGetNextSibling(child)) {
			if (strcmp(mxmlGetElement(child), "parameter")) {
				fprintf(stderr, "can't parse child %s.\n", mxmlGetElement(child));
				continue;
			}

			const char *param = mxmlElementGetAttr(child, "name");
			const char *val = mxmlGetText(child, 0);
			if (!param || !val) {
				continue;
			}

			if (!strcmp(param, "LOFFSET")) {
				sscanf(val, "0x%04hX", &s_aCalib[id].loffset);
			} else if (!strcmp(param, "HOFFSET")) {
				sscanf(val, "0x%04hX", &s_aCalib[id].hoffset);
			}
		}
	}

	mxmlDelete(xml);

	return 0;
}

int rkGenCalibFile(const char *path)
{
	int id, rc;
	FILE *fp;
	char tmp[16];
	mxml_node_t *xml, *calib, *group, *node;

	fp = fopen(path, "w+");
	if (fp == NULL) {
		fprintf(stderr, "%s : Can't open file \'%s\' : %s.\n", __func__, path, strerror(errno));
		return -1;
	}

	xml = mxmlNewXML("1.0");
	calib = mxmlNewElement(xml, "calibrate");

	for (id = 0; id < AI_NUM; id++) {
		if (!s_aCalib[id].inuse) {
			continue;
		}

		group = mxmlNewElement(calib, "group");
		sprintf(tmp, "CH%d", id);
		mxmlElementSetAttr(group, "id", tmp);

		node = mxmlNewElement(group, "parameter");
		mxmlElementSetAttr(node, "name", "LOFFSET");
		sprintf(tmp, "0x%04X", s_aCalib[id].loffset);
		mxmlNewText(node, 0, tmp);

		node = mxmlNewElement(group, "parameter");
		mxmlElementSetAttr(node, "name", "HOFFSET");
		sprintf(tmp, "0x%04X", s_aCalib[id].hoffset);
		mxmlNewText(node, 0, tmp);
	}

	rc = mxmlSaveFile(xml, fp, rkCalibXmlSaveCallBack);
	if (rc == -1) {
		fprintf(stderr, "%s : save \'%s\' failed : %s.\n", __func__, path, strerror(errno));
	}

	mxmlDelete(xml);
	fclose(fp);

	return rc;
}

int rkCalibExec(int id, int type)
{
	int rc, fd, cnt, errcnt = 0, sum = 0;
	char dev[64];
	uint16_t val;

	sprintf(dev, "%s%d", AI_DEV_NAME_PREFIX, id);

	fd = open(dev, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s : open \'%s\' failed.\n", __func__, dev);
		return -1; 
	}   

	for (cnt = 0; cnt < 50; cnt++) {
		if (errcnt > 50) {
			close(fd);
			return -1;
		}

		rc = read(fd, &val, sizeof(uint16_t));
		if (rc != sizeof(uint16_t)) {
			fprintf(stderr, "%s : read \'%s\' failed.\n", __func__, dev);
			close(fd);
			return -1; 
		}
		if (type == 0 && val > 0xFF) {
			errcnt++;
			cnt--;
			continue;
		} else if (type != 0 && val < 0xFF) {
			cnt--;
			errcnt++;
			continue;
		}
		//printf("val[%d] = 0X%04X\n", cnt, val);
		sum += val;
	}

	close(fd);

	if (type == 0) {
		s_aCalib[id].loffset = sum / cnt;
	} else {
		s_aCalib[id].hoffset = sum / cnt;
	}
	s_aCalib[id].inuse = 1;

	return 0;
}

int rkCliCalib()
{
	char input[64], path[128];
	int channel, rc, flag;

	printf("Begin to calibrate analog input.\n");
	printf("You can type \"quit\" to exit during calibrating.\n");
	printf("Now please connect your calibrator to the analog expand board.\n");
	printf("And type the channel number which you want to calibrate\n"
			"according to the prompt.\n");
	printf("Are you all ready? let's go!!!\n");

	rkCalibInit();
	while(1) {
		printf("Which Channel Do You Want To Calibrate(1 - %d) ? :", AI_NUM);
		fflush(stdout);
		rc = scanf("%s", input);

		channel = atoi(input);
		if (channel < 1 && channel > 24) {
			puts("Error : Wrong Input!!!\n");
			puts("Type \"quit\" to exit.");
			continue;
		} else if (!strcasecmp(input, "quit")) {
			break;
		}

		do {
			printf("Now we will calibrate channel %d's low level value\n", channel);
			printf("Please turn your calibrator mode to 'mA' level,\n"
					"and generate 0mA current.\n");
			printf("Are you ready?(Y/N):");
			fflush(stdout);
			rc = scanf("%s", input);
			if (!strcasecmp(input, "quit")) {
				flag = 0;
			} else if (!strcasecmp(input, "n") || !strcasecmp(input, "no")) {
				flag = 1;
			} else if (!strcasecmp(input, "y") || !strcasecmp(input, "yes")) {
				printf("Calibrating! Please Wait...\n");
				rc = rkCalibExec(channel - 1, 0);
				if (rc == -1) {
					printf("Failed to calibrate!\n");
					flag = -1;
				}
				flag = 2;
			} else {
				puts("Error : Wrong Input!!!\n");
				flag = -1;
			}
		} while(flag == -1);

		if (flag == 0) {
			break;
		} else if (flag == 1) {
			continue;
		}

		do {
			printf("Now we will calibrate channel %d's high level value.\n", channel);
			printf("Please turn your calibrator mode to 'mA' level,\n"
					"and generate 20mA current.\n");
			printf("Are you ready?(Y/N):");
			fflush(stdout);
			rc = scanf("%s", input);
			if (!strcasecmp(input, "quit")) {
				flag = 0;
			} else if (!strcasecmp(input, "n") || !strcasecmp(input, "no")) {
				flag = 1;
			} else if (!strcasecmp(input, "y") || !strcasecmp(input, "yes")) {
				printf("Calibrating! Please Wait...\n");
				rc = rkCalibExec(channel - 1, 1);
				if (rc == -1) {
					printf("Failed to calibrate!\n");
					flag = -1;
				}
				flag = 2;
			} else {
				puts("Error : Wrong Input!!!\n");
				flag = -1;
			}
		} while(flag == -1);

		if (flag == 0) {
			break;
		} else if (flag == 1) {
			continue;
		}
	}

	sprintf(path, "%s/%s", DEF_CFG_FILE_DIR, AI_CALIB_FILE);
	rc = rkGenCalibFile(path);
	if (rc == 0) {
		printf("Generate calibrate file \"%s\".\n", path);
	}

	return 0;
}
