#ifndef _HANDLER_BUDDY_H_
#define _HANDLER_BUDDY_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：handler_func.h
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
	//void get_recent_chat(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_recent_chat(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_frequent_chat(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_single_user_info(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_user_info(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_all_user_info(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_user_avatar(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_user_sign_info(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_friend_remark(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_friend_req_list(task *ptask, MYSQL *mysql, cache_conn *redis);
};

#endif