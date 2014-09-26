
ARCH = x86

ifeq ($(ARCH), arm)
CC = /usr/local/arm/4.3.3-rockontrol/bin/arm-linux-gcc
else
CC = gcc
endif

DEST = tcpserver 
OBJS = main.o

LDFLAGS =
CFLAGS = -g

all : $(DEST)

$(DEST) : $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJS) : %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY : clean

clean :
	rm -f $(DEST) $(OBJS)
