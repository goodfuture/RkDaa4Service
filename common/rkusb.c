/*******************************************************************************
 * Copyright(C)	    : 2013 Rockontrol Industrial Park, Inc.
 * Version          : V1.0
 * Author           : KangQi
 * Tel              : 18636181581/6581
 * Email            : goodfuturemail@gmail.com
 * File Name        : rkusb.c
 * Created At       : 2013-09-23 17:44
 * Last Modified    : 2013-09-23 17:44
 * Description      : 
 * History	        : 
*******************************************************************************/
#include "rkusb.h"
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>

int rkUsbDiskCheck(struct usbdisk *dev)
{
	int rc;
	FILE *fp;
	char buf[1024];
	char *line, *ptr;

	rc = access(USB_DEV_CHECK, F_OK);
	if (rc == -1) {
		fprintf(stderr, "%s : %s.\n", __func__, strerror(errno));
		return -1;
	}

	fp = fopen(DEV_PART_FILE, "r");
	if (!fp) {
		fprintf(stderr, "%s : %s.\n", __func__, strerror(errno));
		return -1;
	}

	rc = fread(buf, 1, sizeof(buf), fp);
	fclose(fp);
	if (rc > 0) {
		line = strtok(buf, "\n");
		while(line) {
			ptr = strstr(line, "sd");
			if (ptr && islower(ptr[2]) && isdigit(ptr[3])) {
				sprintf(dev->devnode, "/dev/%s", ptr);
				return 0;
			}
			line = strtok(NULL, "\n");
		}
	}

	return -1;
}

int rkUsbDiskMnt(struct usbdisk *dev)
{
	int rc;
	FILE *fp;
	char buf[256];
	char *ptr;

	fp = fopen(FILE_MNT_CHECK, "r");
	if (!fp) {
		fprintf(stderr, "%s : open file '%s' failed. %s.\n", __func__, FILE_MNT_CHECK, strerror(errno));
		return -1;
	}

	while((ptr = fgets(buf, sizeof(buf), fp)) != NULL) {
		ptr = strtok(ptr, " ");
		if (!strcmp(dev->devnode, ptr)) {
			ptr = strtok(NULL, " ");
			sprintf(dev->mntdir, "%s", ptr);
			fclose(fp);
			return 0;
		}
	}
	fclose(fp);

	if (!strlen(dev->mntdir)) {
		sprintf(dev->mntdir, "%s", DEF_MNT_DIR);
	}

	rc = mount(dev->devnode, dev->mntdir, "vfat", MS_MGC_VAL | MS_SYNCHRONOUS, "codepage=936,iocharset=gb2312");
	if (rc == -1) {
		fprintf(stderr, "%s : mount '%s' on '%s' failed. %s.\n", __func__, dev->devnode, dev->mntdir, strerror(errno));
		return -1;
	}

	return -1;
}

int rkUsbDiskRemove(struct usbdisk *dev)
{
	int rc;

	rc = umount(dev->mntdir);
	if (rc == -1) {
		fprintf(stderr, "%s : %s.\n", __func__, strerror(errno));
		return -1;
	}

	return 0;
}
