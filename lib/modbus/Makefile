ARCH = arm

OBJS = rkmodbus.o rkcrc.o rkser.o

TARGET = libmodbus.so

ifeq ($(ARCH), arm)
CC = /usr/local/arm/3.4.3-rockontrol/bin/arm-linux-gcc
else
CC = gcc
endif

CFLAGS = -Wall -DDEBUG -I../../src -I../../depend/include -I../../common -c

vpath %.c ../../src ./src
vpath %.h 

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -fpic -shared -o $@ $^ $(LDFLAGS) 

$(OBJS) : %.o : %.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY : clean

clean :
	rm -f $(TARGET) $(OBJS)
