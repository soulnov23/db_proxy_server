CC = g++
TARGET_CXXFLAGS = -g -Wall --std=c++11
TARGET_INCLUDE = -I/usr/local/include -I/usr/local/include/log4cpp -I/usr/local/include/google/protobuf -I/home/zhangchengpeng/mysql5.6/include/
MODULE := src

SRC := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp, %.o, $(SRC))

all : $(MODULE)
	@echo "compile src!"

$(MODULE) : $(OBJS)
	@cp $(OBJS) $(OBJ_DIR)

%.o : %.cpp
	$(CC) $(TARGET_CXXFLAGS) -c $^ $(TARGET_INCLUDE)
	
.PHONY : clean

clean :
	rm -rf $(OBJS) *.bak

