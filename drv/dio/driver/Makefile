ifneq ($(KERNELRELEASE),)
	obj-m := dio.o
	
else
	KERNELDIR ?= /home/bright/linux-2.6.29.1
	#KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
	
default:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-linux-
	#$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules
	
endif

clean:
	$(shell rm -f *.bak)
	$(shell rm -f *.o)
	$(shell rm -f *.ko)
	$(shell rm -f *.mod.c)
	$(shell rm -f *.mod.o)
