#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "rkhjt.h"

int main(int argc, char *argv[])
{
	int listenfd, connectfd, ret;
	struct sockaddr_in addr;
	char buf[1024], tmp[8];

	listenfd = socket(PF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) {
		fprintf(stderr, "create socket failed : %s.\n", strerror(errno));
		return -1;
	}

	ret = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret)) == -1) {
		fprintf(stderr, "setsockopt failed : %s.\n", strerror(errno));
		close(listenfd);
		return -1;
	}

	addr.sin_family = PF_INET;
	addr.sin_port = htons(1234);
	addr.sin_addr.s_addr = INADDR_ANY; 

	ret = bind(listenfd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret == -1) {
		fprintf(stderr, "bind failed : %s.\n", strerror(errno));
		return -1;
	}

	ret = listen(listenfd, 10);
	if (ret == -1) {
		fprintf(stderr, "listen failed : %s.\n", strerror(errno));
		return -1;
	}

	ret = sizeof(addr);
	connectfd = accept(listenfd, (struct sockaddr *)&addr, &ret);
	printf("connect with %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	struct hjtMsg msg;
	sprintf(msg.qn, "12345678901234567");
	sprintf(msg.pw, "abcdef");
	sprintf(msg.mn, "98765432101234");
	msg.cn = 1011;
	msg.st = 32;
	msg.flag = 0x01;
	sprintf(buf, "##0000QN=%s;Flag=%c;ST=%02d;CN=%04d;MN=%s;PW=%s;CP=&&&&", msg.qn, msg.flag, msg.st, msg.cn, msg.mn, msg.pw);
	printf("strlen(buf) == %d\n", strlen(buf));
	sprintf(tmp, "%04d", strlen(buf + 6));
	memcpy(buf + 2, tmp, 4);
	printf("strlen(buf) == %d\n", strlen(buf));
	int crc = rkCrc16(buf + 6, strlen(buf + 6));
	sprintf(buf, "%s%04X", buf, crc);
	printf("strlen(buf) == %d\n", strlen(buf));
	strcat(buf, "\r\n");
	printf("strlen(buf) == %d\n", strlen(buf));

	while(1) {
		char tmp[512];
		sleep(1);
		ret = send(connectfd, buf, strlen(buf), 0);
		printf("ret = %d\n", ret);
		recv(connectfd, tmp, sizeof(tmp), 0);
		printf("recv from %s : %s.", inet_ntoa(addr.sin_addr), buf);
		fflush(stdout);
	}

	return 0;
}
