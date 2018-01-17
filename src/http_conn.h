#ifndef _HTTP_CONN_H_
#define _HTTP_CONN_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：http_conn.h
 *描		述：http网络连接对象类
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2016-11-23 14:00
 *修改日期：2016-11-23
*******************************************************************************************/
#include "event.h"
#include <string>
using namespace std;

class http_server;

class http_conn
{
public:
	http_conn(evutil_socket_t fd, string ip, struct event *ev, void *arg);
	~http_conn();

private:
	void free();
	
public:
	evutil_socket_t m_fd;
	string m_ip;
	struct event *m_event;
	http_server *m_http_server;
	void *m_arg;
};

#endif