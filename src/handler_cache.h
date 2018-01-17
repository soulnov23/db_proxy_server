#ifndef _HANDLER_CACHE_H_
#define _HANDLER_CACHE_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：handler_cache.h
 *描		述：Redis更新处理
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
	void update_user_id(task *ptask, MYSQL *mysql, cache_conn *redis);
	int redis_user_id(uint32_t company_id, MYSQL *mysql, cache_conn *redis);
	void update_user_info(task *ptask, MYSQL *mysql, cache_conn *redis);
	int redis_user_info(uint32_t company_id, uint32_t user_id, MYSQL *mysql, cache_conn *redis);
	void update_user_friend_list(task *ptask, MYSQL *mysql, cache_conn *redis);
	int redis_user_friend_list(uint32_t company_id, uint32_t user_id, MYSQL *mysql, cache_conn *redis);
	void update_user_group_list(task *ptask, MYSQL *mysql, cache_conn *redis);
	int redis_user_group_list(uint32_t company_id, uint32_t user_id, MYSQL *mysql, cache_conn *redis);
	void update_group_member_list(task *ptask, MYSQL *mysql, cache_conn *redis);
	int redis_group_member_list(uint32_t company_id, uint32_t group_id, MYSQL *mysql, cache_conn *redis);
	void update_org_tree(task *ptask, MYSQL *mysql, cache_conn *redis);
	int redis_org_tree(uint32_t company_id, MYSQL *mysql, cache_conn *redis);
};

#endif