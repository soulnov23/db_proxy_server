CC = g++
TARGET_LDFLAGS = -L/usr/local/lib -L/home/zhangchengpeng/mysql5.6/lib/ -levent -levent_pthreads -lmysqlclient -ldl -lprotobuf -lpthread -lrt -llog4cpp
OBJS := $(wildcard *.o)

all : $(OBJS)
	$(CC) $^ -o $(TARGET_NAME) $(TARGET_LDFLAGS)
	@cp -f $(TARGET_NAME) ../

.PHONY : clean

clean :
	rm -rf $(TARGET_NAME) *.o *.bak

