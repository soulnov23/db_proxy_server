#ifndef _HANDLER_GROUP_H_
#define _HANDLER_GROUP_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：handler_group.h
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
	void get_group_list(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_group_list(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_group_disturb(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_group_owner(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_group_info(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_group_info(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_group_members(task *ptask, MYSQL *mysql, cache_conn *redis);
	//void get_group_member_list(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_group_create(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_group_dissolve(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_group_change(task *ptask, MYSQL *mysql, cache_conn *redis);
};

#endif