#ifndef _HANDLER_ROUTER_H_
#define _HANDLER_ROUTER_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：handler_router.h
 *描		述：数据库消息处理方法
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2016-06-27 14:00
 *修改日期：2016-06-27
*******************************************************************************************/
#include "task.h"
#include "mysql.h"
#include "cache_conn.h"

namespace db_proxy
{
	void get_register_rsp(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_heart_beat(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_user_state(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_router_table(task *ptask, MYSQL *mysql, cache_conn *redis);
};

#endif