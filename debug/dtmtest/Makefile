
ARCH = x86

ifeq ($(ARCH), arm)
CC = /usr/local/arm/4.3.3-rockontrol/bin/arm-linux-gcc
else
CC = gcc
endif

DEST = tcpserver 
OBJS = main.o rkhjt.o rkcrc.o rkxml.o

LDFLAGS = -lmxml -lpthread
CFLAGS = -g -I../../include

vpath %.c ../../src

all : $(DEST)

$(DEST) : $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJS) : %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY : clean

clean :
	rm -f $(DEST) $(OBJS)
