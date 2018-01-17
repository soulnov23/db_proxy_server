#ifndef _HANDLER_SERVER_H_
#define _HANDLER_SERVER_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：handler_login.h
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
	void get_login_result(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_modify_pwd(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_buddy_config(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_friend_req(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_friend_req_result(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_friend_manage(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_add_recent_chat(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_del_recent_chat(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_online_report(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_msg_report(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_user_role(task *ptask, MYSQL *mysql, cache_conn *redis);
};

#endif