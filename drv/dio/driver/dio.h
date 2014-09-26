/*
 * dio.h: DIO驱动的头文件，同时也作为用户空间调用驱动需要包含的头文件
*/

#ifndef _DIO_H
#define _DIO_H

#include <linux/ioctl.h>


#define DIO_IOC_MAGIC 't'


#define DIO_IOC_DOHIGH 	_IOW(DIO_IOC_MAGIC, 1, unsigned char)
#define DIO_IOC_DOLOW 	_IOW(DIO_IOC_MAGIC, 2, unsigned char)
#define DIO_IOC_GETDO 	_IOR(DIO_IOC_MAGIC, 3, unsigned char)

#endif
