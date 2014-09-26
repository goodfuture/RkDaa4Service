

CC = arm-linux-gcc

EXE := test_dio

SRCS := $(wildcard *.c)

OBJS := $(patsubst %.c, %.o, $(SRCS))

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
    
$(OBJS) : %.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $< $(HEADFILES)
 
.PHONY: clean
clean:
	@rm -rf $(OBJS) $(EXE)
