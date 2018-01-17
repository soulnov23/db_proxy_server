CC = g++
PROJ_PATH = $(shell pwd)
TARGET_NAME = db_proxy_server
MODULES = hiredis http_parser jsoncpp protobuf src
OBJ_DIR = $(PROJ_PATH)/objs

.PHONY : clean $(MODULES)

all : $(MODULES)
	$(MAKE) -C $(OBJ_DIR) TARGET_NAME="$(TARGET_NAME)" all
	@echo "make done!"

hiredis :
	$(MAKE) -C $(PROJ_PATH)/hiredis OBJ_DIR="$(OBJ_DIR)" all

http_parser :
	$(MAKE) -C $(PROJ_PATH)/http_parser OBJ_DIR="$(OBJ_DIR)" all

jsoncpp :
	$(MAKE) -C $(PROJ_PATH)/jsoncpp OBJ_DIR="$(OBJ_DIR)" all

protobuf :
	$(MAKE) -C $(PROJ_PATH)/protobuf OBJ_DIR="$(OBJ_DIR)" all

src :
	$(MAKE) -C $(PROJ_PATH)/src OBJ_DIR="$(OBJ_DIR)" all

clean :
	$(MAKE) -C $(PROJ_PATH)/hiredis clean
	$(MAKE) -C $(PROJ_PATH)/http_parser clean
	$(MAKE) -C $(PROJ_PATH)/jsoncpp clean
	$(MAKE) -C $(PROJ_PATH)/protobuf clean
	$(MAKE) -C $(PROJ_PATH)/objs TARGET_NAME="$(TARGET_NAME)" clean
	$(MAKE) -C $(PROJ_PATH)/src clean
	rm -rf $(TARGET_NAME) core.*
	@echo "clean done!"

