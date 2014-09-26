#include "string.h"
#include "stdio.h"      /*标准输入输出定义*/
#include "stdlib.h"     /*标准函数库定义*/
#include "unistd.h"     /*Unix 标准函数定义*/
#include "sys/types.h" 
#include "sys/stat.h"   
#include "fcntl.h"      /*文件控制定义*/
#include "termios.h"    /*PPSIX 终端控制定义*/
#include "errno.h"      /*错误号定义*/


int main(int argc,char *argv[])
{
	char str[20];
	int i,ret,index,fd[2];
	int buf[3];
	unsigned char wdata;
	unsigned short rdata;
			
	if(argc!=2)
	{
		printf("param err.\n");
		return 0;
	}
	
	wdata = atoi(argv[1]);
		
	sprintf(str,"/dev/dio");
	fd[0] = open(str, O_RDWR);
	if (-1 == fd[0])
	{
		perror("open dio.");  
		return -1;
	}
	
	ret = write(fd[0], &wdata, sizeof(wdata));
	printf("write done.\n");
	
	while(1)
	{
		ret = read(fd[0], &rdata, sizeof(rdata));
		printf("read val: %.4x\n",rdata);
		sleep(1);
	}
		
	close(fd[0]);
		
	return 0;
}
