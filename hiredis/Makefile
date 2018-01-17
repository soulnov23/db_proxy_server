C = gcc
TARGET_CFLAGS = -g -Wall
MODULE := hiredis

SRC := $(wildcard *.c)
OBJS := $(patsubst %.c, %.o, $(SRC))

all : $(MODULE)
	@echo "compile hiredis!"

$(MODULE) : $(OBJS)
	@cp $(OBJS) $(OBJ_DIR)

%.o : %.c
	$(C) $(TARGET_CFLAGS) -c $^

.PHONY : clean

clean :
	rm -rf $(OBJS) *.bak

