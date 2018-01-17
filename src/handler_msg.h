#ifndef _HANDLER_MSG_H_
#define _HANDLER_MSG_H_
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
	void set_offline_msg_data(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_online_msg_data(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_recall_msg_data(task *ptask, MYSQL *mysql, cache_conn *redis);
	void set_receipt_status(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_receipt_list(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_offline_msg_cnt(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_group_offline_msg_cnt(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_offline_msg(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_history_msg(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_latest_msg_id(task *ptask, MYSQL *mysql, cache_conn *redis);
	void get_msg_by_id(task *ptask, MYSQL *mysql, cache_conn *redis);
};

#endif