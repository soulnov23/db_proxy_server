C = g++
TARGET_CFLAGS = -g -Wall
TARGET_INCLUDE = -I/usr/local/include/google/protobuf
MODULE := protobuf

SRC := $(wildcard *.cc)
OBJS := $(patsubst %.cc, %.o, $(SRC))

all : $(MODULE)
	@echo "compile protobuf!"

$(MODULE) : $(OBJS)
	@cp $(OBJS) $(OBJ_DIR)

%.o : %.cc
	$(C) $(TARGET_CFLAGS) -c $^ $(TARGET_INCLUDE)

.PHONY : clean

clean :
	rm -rf $(OBJS) *.bak

