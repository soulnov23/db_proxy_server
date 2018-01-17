#ifndef _HANDLER_ACCOUNT_H_
#define _HANDLER_ACCOUNT_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：handler_login.h
 *描		述：数据库消息处理方法
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2017-04-14 14:00
 *修改日期：2017-04-14
*******************************************************************************************/
#include "mysql.h"
#include "cache_conn.h"
#include "event2/util.h"
#include "../jsoncpp/json/json.h"

void account_app(Json::Value &value);

void account_init(Json::Value &value);

void account_login(evutil_socket_t fd, Json::Value &value, MYSQL *mysql, cache_conn *redis);

#endif