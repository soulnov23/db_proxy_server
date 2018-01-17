#ifndef _NET_CONN_H_
#define _NET_CONN_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：net_conn.h
 *描		述：网络连接对象类
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2016-11-10 9:00
 *修改日期：2016-11-10
*******************************************************************************************/
#include "event.h"
#include <string>
using namespace std;

class net_conn
{
public:
	net_conn(evutil_socket_t fd, string ip, struct event *ev, void *arg);
	~net_conn();

private:
	void free();
	
public:
	evutil_socket_t m_fd;
	string m_ip;
	struct event *m_event;
	struct evbuffer *m_evbuffer;
	void *m_arg;
};

#endif