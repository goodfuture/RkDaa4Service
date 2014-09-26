ARCH = arm

OBJS = rkmodbus.o rkcrc.o rkser.o

CFLAGS = -Wall -DDEBUG -I../src -I../depend/include -I../common -c

TARGET = libmodbus.so

ifeq ($(ARCH), arm)
CC = /usr/local/arm/3.4.3-rockontrol/bin/arm-linux-gcc
else
CC = gcc
endif

vpath %.c ../src
vpath %.h ../include

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -fpic -shared -o $@ $^ $(LDFLAGS) 

$(OBJS) : %.o : %.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY : clean

clean :
	rm -f $(TARGET) $(OBJS)
