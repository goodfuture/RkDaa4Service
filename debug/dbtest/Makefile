
ARCH = arm

ifeq ($(ARCH), arm)
CC = /usr/local/arm/4.3.3-rockontrol/bin/arm-linux-gcc
else
CC = gcc
endif

#DEST = fmltest
#SRCS = rkfml.c fmltest.c
DEST = sDebug
OBJS = bugtest.o rkdam.o rkxml.o rkserial.o rkfml.o rkshm.o rkfifo.o

LDFLAGS = -lpthread -lm -lrt -ldl -L../depend/lib -lmxml #-lm -lreadline -ltermcap 
CFLAGS = -g -DDEBUG -I./include -I../include -I../depend/include

vpath %.c ./src
vpath %.c ../src
vpath %.h ../include
vpath %.h ../depend/include
vpath %.h ./include

all : $(DEST)

$(DEST) : $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJS) : %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY : clean

clean :
	rm -f $(DEST) $(OBJS)
