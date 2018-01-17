#ifndef _JSON_TASK_H_
#define _JSON_TASK_H_
/*****************************************************************************************
*Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
*文件名称：json_task.h
*描		述：数据库消息任务类
*当前版本：1.0
*作		者：zhangcp
*创建日期：2017-04-10 14:00
*修改日期：2017-04-10
*******************************************************************************************/
#include "task.h"

class json_task : public task
{
public:
	json_task(evutil_socket_t fd, char *msg, int length);
	~json_task();

	virtual void run(MYSQL *mysql, cache_conn *redis);
	virtual void send();
	
public:
	evutil_socket_t m_fd;
	char *m_msg;
	int m_length;
};

#endif