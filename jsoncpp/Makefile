CC = g++
TARGET_CXXFLAGS = -g -Wall
MODULE := jsoncpp

SRC := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SRC))

all : $(MODULE)
	@echo "compile jsoncpp!"

$(MODULE) : $(OBJS)
	@cp $(OBJS) $(OBJ_DIR)

%.o : %.cpp
	$(CC) $(TARGET_CXXFLAGS) -c $^

.PHONY : clean

clean :
	rm -rf $(OBJS) *.bak

