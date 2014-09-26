
/*
 * dio.c: dio的驱动文件。
 *     
 * 此驱动既可以以模块的形式加载，也可以编译到内核中；并且对于使用者两者没有区别。
 *     
 * 当编译到内核时：只需要将本驱动文件放入内核drivers/char/目录中，并且修改makefile和
 * Kconfig文件相关内容，添加相应编译开关即可。
 * 
 * 用户使用时，首先应在/dev目录下新建一个字符型的设备文件，主设备号为DIO_MAJOR，即206，
 * 次设备号为0. 
 *     
 * 当对设备文件进行读操作时，返回的是当前16路数字量输入的值，因此用户传入的数据存放缓冲区
 * 应该是一个16位的unsigned short型数据，返回数据的每一位对应相应的输入状态，0代表输入
 * 的是低电平，1代表高电平
 * 
 * 当对设备文件进行写操作时，驱动将根据用户传入的unsigned char型数据设置8路数字量输出的值，
 * 每一位对应一路输出，其中0代表输出低电平，1代表输出高电平
 * 
 * 如果只是希望单独设置某一路或几路的数字量输出值可以使用ioctl操作。
 * 参数DIO_IOC_DOHIGH表示将输出设置为高电平，驱动会根据用户传入的unsigned char型数据
 * 设置8路数字量输出的值，每一位对应一路输出，其中0代表不更改当前的输出值，1代表输出高电平。
 * 参数DIO_IOC_DOLOW表示将输出设置为低电平，驱动会根据用户传入的unsigned char型数据
 * 设置8路数字量输出的值，每一位对应一路输出，其中0代表不更改当前的输出值，1代表输出低电平。
 * 
 * 修改历史：
 * 2011.1.27
 *     1. 增加DIO_IOC_GETDO操作，获得当前的DO输出值
 2012.9.14
 1. 增加sample_order变量，确定数字量的采集顺序 1.0版本的值为0，2.0版本的为1，默认的为0  
 */


#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/spi/spi.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/clk.h>
#include <asm/uaccess.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <mach/at91_pmc.h>
#include <mach/at91_pio.h>
#include "dio.h"

extern void led_onoff(int type,unsigned char* value);
static int use_led = 0;			//是否使用led的模块参数，为0是不使用，1是使用
static int sample_order = 1;		//数字量的采集顺序，为0时是1.0，为1时是2.0
module_param(use_led, int, S_IRUGO);
module_param(sample_order, int, S_IRUGO);
//#define MYDEBUG
#ifdef MYDEBUG
#define PRINT_FUNC_NAME { 		\
	printk(__FUNCTION__); 	\
	printk("\n"); 			\
}
#else
#define PRINT_FUNC_NAME
#endif


#define DIO_MAJOR 206	//主设备号


typedef struct _DIO_DEVICE_{
	struct cdev cdev;
} DIO_DEVICE;

static DIO_DEVICE dio_device;

//////////////////////////////////////////////////////
static u32 led_val = 0;

void flush_led(void)
{
	static u32 ledlastval;
	u8 led[4];
	if(use_led == 0)
		return;

	if(ledlastval == led_val)
		return;
	ledlastval = led_val;
	led[3]=(led_val>>24)&0xff;
	led[2]=(led_val>>16)&0xff;
	led[1]=(led_val>>8)&0xff;
	led[0]=(led_val)&0xff;
	led[0]=~led[0];   //DI进行取反
	led[1]=~led[1];
	led_onoff(0,led);	


}

//////////////////
int dio_open(struct inode *inode,struct file *filp)
{
	PRINT_FUNC_NAME
		filp->private_data = &dio_device;
	return 0;
}

static int dio_ioctl(struct inode *inode,struct file *filp,unsigned int cmd,unsigned long arg)
{
	unsigned char val;
	int ret;

	PRINT_FUNC_NAME

		switch(cmd){
			case DIO_IOC_DOHIGH:
				val = arg;
				at91_sys_write(AT91_PIOE+PIO_SODR,val<<8);

				at91_sys_write(AT91_PIOE+PIO_CODR,(1<<20));		//上升沿赋值
				at91_sys_write(AT91_PIOE+PIO_SODR,(1<<20));
				break;
			case DIO_IOC_DOLOW:
				val = arg;
				at91_sys_write(AT91_PIOE+PIO_CODR,val<<8);

				at91_sys_write(AT91_PIOE+PIO_CODR,(1<<20));		//上升沿赋值
				at91_sys_write(AT91_PIOE+PIO_SODR,(1<<20));
				break;
			case DIO_IOC_GETDO:
				val = ((at91_sys_read(AT91_PIOE+PIO_ODSR)>>8) & 0xff);
				ret = copy_to_user((unsigned char __user *)arg, &val, sizeof(char));
				break;
			default:
				break;	
		}

	return 0;
}

static ssize_t dio_read(struct file *filp,char __user *buf,size_t size,loff_t *ppos)
{
	unsigned short data=0;
	int ret;
	unsigned short data_order=0,lrl=1,tempv,i=0;
	PRINT_FUNC_NAME

		if(size != sizeof(unsigned short))
			return -EINVAL;

	at91_sys_write(AT91_PIOE+PIO_SODR,(1<<16)+(1<<17));
	at91_sys_write(AT91_PIOE+PIO_CODR,(1<<16));
	data = (at91_sys_read(AT91_PIOE+PIO_PDSR) & 0xff);
	at91_sys_write(AT91_PIOE+PIO_SODR,(1<<16)+(1<<17));
	at91_sys_write(AT91_PIOE+PIO_CODR,(1<<17));
	data += (unsigned short)((at91_sys_read(AT91_PIOE+PIO_PDSR) & 0xff) << 8);
	at91_sys_write(AT91_PIOE+PIO_SODR,(1<<16)+(1<<17));
	tempv=data;
	if(sample_order==1)
	{
		for(i=0;i<16;i++)
		{
			data &= lrl<<i;
			if(data)
			{
				data_order |= (0x8000>>i);
			}		
			data = tempv;  
		}
		ret = copy_to_user(buf,&led_val,sizeof(unsigned short)*2);    
		led_val = (led_val & 0xffff0000) + data_order;
	}
	else
	{
		ret = copy_to_user(buf,&data,sizeof(unsigned short));
		led_val = (led_val & 0xffff0000) + data;
	}


	flush_led();

	return size;
}

static ssize_t dio_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	unsigned char data;
	int ret;

	PRINT_FUNC_NAME

		ret = copy_from_user(&data,buf,sizeof(unsigned char));	//用户传入的数据应该是unsigned char
	led_val = (led_val & 0x0000ffff) + (((u32)data)<<24);

	at91_sys_write(AT91_PIOE+PIO_OWER,(0xff<<8));
	at91_sys_write(AT91_PIOE+PIO_ODSR,(data<<8));
	at91_sys_write(AT91_PIOE+PIO_OWDR,(0xff<<8));

	at91_sys_write(AT91_PIOE+PIO_CODR,(1<<20));		//上升沿赋值
	at91_sys_write(AT91_PIOE+PIO_SODR,(1<<20));

	//led_val = (led_val & 0x0000ffff) + (((u32)5500)<<16);

	flush_led();

	return size;
}

static int dio_release(struct inode *inode,struct file *filp)
{
	PRINT_FUNC_NAME
		return 0;
}

static const struct file_operations dio_fops ={
	.owner	= THIS_MODULE,
	.read 	= dio_read,
	.write 	= dio_write,
	.ioctl	= dio_ioctl,
	.open	= dio_open,
	.release = dio_release,
};

////////////////////////////////////////
static int dio_char_register(void)
{
	int err,i;
	dev_t devno;

	devno = MKDEV(DIO_MAJOR,0);

	register_chrdev_region(devno,1,"dio"); //release the device id

	cdev_init(&(dio_device.cdev),&dio_fops);
	dio_device.cdev.owner = THIS_MODULE;
	dio_device.cdev.ops = &dio_fops;
	err = cdev_add(&(dio_device.cdev), devno, 1);
	if(err)
		return -EINVAL;

	////////////////	
	at91_sys_write(AT91_PMC_PCER, 1<<AT91SAM9263_ID_PIOCDE); //enable pmc

	//set gpio
	for(i=0; i<8; ++i)
	{
		at91_set_gpio_input(AT91_PIN_PE0+i,0);
		at91_set_deglitch(AT91_PIN_PE0+i,0); 
	}
	for(i=0; i<8; ++i)
		at91_set_gpio_output(AT91_PIN_PE8+i,1);

	at91_set_gpio_output(AT91_PIN_PE20,1);	
	at91_set_gpio_output(AT91_PIN_PE16,1);
	at91_set_gpio_output(AT91_PIN_PE17,1);	
	at91_sys_write(AT91_PIOE+PIO_SODR,(1<<16)+(1<<17));
	//////////////////	set led
	at91_set_gpio_output(AT91_PIN_PB30,1); //data in1
	at91_set_gpio_output(AT91_PIN_PB26,1);
	at91_set_gpio_output(AT91_PIN_PB23,1); //slck		
	//////////////////
	return 0;
}

static int dio_char_unregister(void)
{
	dev_t devno;

	devno = MKDEV(DIO_MAJOR,0);

	cdev_del(&(dio_device.cdev));

	unregister_chrdev_region(devno,1); 

	return 0;
}

static int __init dio_init(void)
{	
	int ret;

	ret = dio_char_register();
	if(0 != ret)
	{
		printk("dio register char device failed.\n");
		return ret;
	}

	return 0;
}


static void __exit dio_exit(void)
{
	dio_char_unregister();
}


module_init(dio_init);
module_exit(dio_exit);


MODULE_LICENSE("Dual BSD/GPL");

