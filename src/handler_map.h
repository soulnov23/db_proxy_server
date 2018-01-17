#ifndef _HANDLER_MAP_H_
#define _HANDLER_MAP_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：handler_map.h
 *描		述：数据库消息处理句柄管理
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2016-06-27 14:00
 *修改日期：2016-06-27
*******************************************************************************************/
#include <map>
using namespace std;
#include "task.h"
#include "mysql.h"
#include "cache_conn.h"

typedef void(*msg_handler_t)(task *ptask, MYSQL *mysql, cache_conn *redis);
typedef map<uint16_t, msg_handler_t> handler_map_t;

//构造函数初始化map容器，把现有的cmd_id和对应的接口函数指针存到map中
class handler_map
{
private:
	handler_map();

public:
	~handler_map();
	static handler_map *get_instance();
	msg_handler_t get_handler(uint16_t cmd_id);

private:
	static handler_map *m_instance;
	handler_map_t m_handler_map;
};

#endif