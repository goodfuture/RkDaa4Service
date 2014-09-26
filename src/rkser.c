/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.0
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkser.c
 * Created At		: 2013-09-23 14:29
 * Last Modified	: 2013-10-21 09:29
 * Description		: 
 * History			: 
*******************************************************************************/

#include "rkser.h"
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

int rkSerOpen(struct com *com)
{
	struct termios opt;
	char dev[32];
	int index;
	int speed_arr[] = {B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200, B300};
	int name_arr[] = {115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200, 300};

	if (com->id == DTU_DEV_ID) {
		sprintf(dev, "%s", DTU_DEV_INTERFACE);
	} else {
		sprintf(dev, "%s%d", COM_DEV_NAME_PREFIX, com->id + 5);
	}

	for (index = 0; index < sizeof(name_arr) / sizeof(int); index++) {
		if (name_arr[index] == com->baud) {
			break;
		}
	}

	if (index >= sizeof(name_arr) / sizeof(int)) {
		fprintf(stderr, "%s : (%s)Wrong baud %d.\n", __func__, dev, com->baud);
		return -1;
	}

	com->fd = open(dev, O_RDWR | O_NOCTTY);
	if (com->fd == -1) {
		fprintf(stderr, "%s : Open \'%s\' failed : %s.\n", __func__, dev, strerror(errno));
		return -1;
	}

	tcgetattr(com->fd, &opt);
	cfsetispeed(&opt, speed_arr[index]);
	cfsetospeed(&opt, speed_arr[index]);
	tcsetattr(com->fd, TCSANOW, &opt);
	tcflush(com->fd, TCIOFLUSH);

	tcgetattr(com->fd, &opt);
	opt.c_cflag &= ~CSIZE;                                                                                                            
	switch (com->db) {
		case 7:
			opt.c_cflag |= CS7;
			break;
		case 8:
			opt.c_cflag |= CS8;
			break;
		default:
			fprintf(stderr, "%s : (%s)Wrong data bit %d.\n", __func__, dev, com->db);
			close(com->fd);
			return -1;
	}

	switch (com->sb) {
		case 1:
			opt.c_cflag &= ~CSTOPB;
			break;
		case 2:
			opt.c_cflag |= CSTOPB;
			break;
		default:
			fprintf(stderr, "%s : (%s)Wrong stop bit %d.\n", __func__, dev, com->sb);
			close(com->fd);
			return -1;
	}

	switch (com->parity) {
		case NONE:
			opt.c_cflag &= ~PARENB;
			opt.c_iflag &= ~INPCK;
			break;
		case ODD:
			opt.c_cflag |= (PARODD | PARENB);
			opt.c_iflag |= INPCK;
			break;
		case EVEN:
			opt.c_cflag |= PARENB;
			opt.c_cflag &= ~PARODD;
			opt.c_iflag |= INPCK;
			break;
		default:
			fprintf(stderr, "%s : (%s)Wrong parity.\n", __func__, dev);
			close(com->fd);
			return -1;
	}

	opt.c_cc[VTIME] = 0;
	opt.c_cc[VMIN] = 1;
	tcsetattr(com->fd, TCSANOW, &opt);

	tcgetattr(com->fd, &opt);
	opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	opt.c_iflag &= ~(IXON | IXOFF | ICRNL );
	opt.c_oflag &= ~(OPOST | ONLCR);
	tcsetattr(com->fd, TCSANOW, &opt);

	return com->fd; 
}

int rkSerSend(const char *buf, uint32_t len, int fd)
{
	int cnt;

	if (fd <= 0) return -1;

	cnt = write(fd, buf, len);
	usleep(50000);

	return cnt;
}

int rkSerRecv(char *buf, uint32_t len, int timeout, int fd)
{
	int cnt, ret;
	fd_set rfds;
	struct timeval overtime;

	if(fd < 0) {
		return -1;
	}

	overtime.tv_sec = timeout / 1000;
	overtime.tv_usec = (timeout % 1000) * 1000;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	if(timeout <= 0) {
		ret = select(fd + 1, &rfds, NULL, NULL, NULL);
	} else {
		ret = select(fd + 1, &rfds, NULL, NULL, &overtime);
	}

	if(ret > 0) {
		bzero(buf, len);
		cnt = read(fd, buf, len);
	} else {
		cnt = -1;
	}

	return cnt;
}

int rkSerClose(int fd)
{
	return fd < 0 ? -1 : close(fd);
}
