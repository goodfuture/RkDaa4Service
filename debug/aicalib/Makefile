
ARCH = arm

ifeq ($(ARCH), arm)
CC = /usr/local/arm/4.3.3-rockontrol/bin/arm-linux-gcc
else
CC = gcc
endif

DEST = aicalib
OBJS = aicalib.o rkxml.o 

LDFLAGS = -lpthread -lm -lrt -ldl -L../..//depend/lib -lmxml
CFLAGS = -g -DDEBUG -I../../include -I../../depend/include

vpath %.c ../../src
vpath %.h ../../include
vpath %.h ../../depend/include

all : $(DEST)

$(DEST) : $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(OBJS) : %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY : clean

clean :
	rm -f $(DEST) $(OBJS)
