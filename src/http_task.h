#ifndef _HTTP_TASK_H_
#define _HTTP_TASK_H_
/*****************************************************************************************
*Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
*文件名称：http_task.h
*描		述：数据库消息任务类
*当前版本：1.0
*作		者：zhangcp
*创建日期：2017-04-10 11:00
*修改日期：2017-04-10
*******************************************************************************************/
#include "task.h"

class http_task : public task
{
public:
	http_task(bool flag, evutil_socket_t fd, string *msg);
	~http_task();

	virtual void run(MYSQL *mysql, cache_conn *redis);
	virtual void send();

public:
	bool m_flag;
	evutil_socket_t m_fd;
	string *m_msg;
};

#endif