#ifndef _TASK_H_
#define _TASK_H_
/*****************************************************************************************
*Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
*文件名称：task.h
*描		述：任务类
*当前版本：1.0
*作		者：zhangcp
*创建日期：2016-12-01 14:00
*修改日期：2016-12-01
*******************************************************************************************/
#include "mysql_conn.h"
#include "cache_conn.h"
#include "message.h"
#include "event2/util.h"

class task
{
public:
	task(){}
	virtual ~task(){}

	virtual void *get_msg()
	{
		return NULL;
	}

	virtual evutil_socket_t get_fd()
	{
		return 0;
	}

	virtual void run(MYSQL *mysql, cache_conn *redis) = 0;
	virtual void send() = 0;
};

#endif