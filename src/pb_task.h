#ifndef _PB_TASK_H_
#define _PB_TASK_H_
/*****************************************************************************************
*Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
*文件名称：pb_task.h
*描		述：protobuf消息任务类
*当前版本：1.0
*作		者：zhangcp
*创建日期：2017-04-07 9:00
*修改日期：2017-04-07
*******************************************************************************************/
#include "task.h"

class pb_task : public task
{
public:
	pb_task(evutil_socket_t fd, message *msg);
	~pb_task();

	virtual void run(MYSQL *mysql, cache_conn *redis);
	virtual void send();

	virtual void *get_msg();
	virtual evutil_socket_t get_fd();

public:
	evutil_socket_t m_fd;
	message *m_msg;
};

#endif