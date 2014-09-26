#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define THREAD_NUM 5

int main(int argc, char *argv[])
{
	pthread_t thread[THREAD_NUM];
	int i;

	for (i = 0; i < THREAD_NUM; i++) {
		pthread_create(&thread[i], NULL, );
	}

	for (i = 0; i < THREAD_NUM; i++) {
		pthread_join(thread[i]);
	}
	return 0;
}

void run(void *arg)
{
	while(1) {
		func();
		sleep(1);
	}
}

void func(void)
{
}
