ARCH = arm
#ARCH = x86

TOOLDIR = /usr/local/arm

ifeq ($(ARCH), arm)
CC = $(TOOLDIR)/3.4.3-rockontrol/bin/arm-linux-gcc
CFLAGS = -I./depend/include
LDFLAGS = -L./depend/lib
else
CC = gcc
endif

CFLAGS += -g -I. -I./src -I./common/ -DDEBUG -Wall
LDFLAGS += -lsqlite3 -lmxml -lpthread -ldl -lm -lrt

OBJS = main.o rkshm.o rkfifo.o rkxml.o rkhjt.o rkser.o rkinit.o rkhmi.o rkprotocol.o rkserver.o
OBJS += rkdam.o rkdtm.o rkdsm.o rkfml.o rkcrc.o rkscan.o rkdebug.o rkcou.o rkcalib.o
#DEST = /home/kangqi/Debug/shucaiyi_stable/app/shucaiyi/Core
#DEST = /home/kangqi/Debug/shucaiyi_stable/Core.bin
DEST = Core.bin

vpath %.h src 
vpath %.c src
vpath %.h common
vpath %.c common

all : $(DEST)

$(DEST):$(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS) 

$(OBJS):%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY : clean

clean:
	rm -f ${DEST} $(OBJS) GUIModule sDebug
