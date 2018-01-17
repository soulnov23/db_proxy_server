#ifndef _NET_WORK_H_
#define _NET_WORK_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：net_work.h
 *描		述：网络连接操作类
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2016-06-27 14:00
 *修改日期：2016-06-27
*******************************************************************************************/
#include "event.h"
#include "net_conn.h"
#include "http_conn.h"
#include "config_file_oper.h"

class net_work
{
public:
	net_work();
	virtual ~net_work();

	int start_net_work();

	void do_listen();
	void do_http_listen();

	int do_account_connect();
	int do_http_connect(const char *buf, int len);
	int do_router_connect();
	int do_status_connect();

	void do_account_re_connect();
	void do_router_re_connect();
	void do_status_re_connect();

	void do_dispatch();
	void stop_net_work();

public:
	void set_status_ip(string ip);
	void set_status_port(unsigned int port);

private:
	static void do_http_accept(evutil_socket_t fd, short ev, void *ctx);
	static void do_accept(evutil_socket_t fd, short ev, void *ctx);
	static void do_http_read(evutil_socket_t fd, short ev, void *ctx);
	static void do_read(evutil_socket_t fd, short ev, void *ctx);
	static void do_account_read(evutil_socket_t fd, short ev, void *ctx);

	static void do_account_timer(evutil_socket_t fd, short ev, void *ctx);
	static void do_router_timer(evutil_socket_t fd, short ev, void *ctx);
	static void do_status_timer(evutil_socket_t fd, short ev, void *ctx);

	static void signal_handler_t(int signum);

	virtual void read(evutil_socket_t fd, short ev, void *ctx) = 0;
	virtual void http_read(evutil_socket_t fd, short ev, void *ctx) = 0;
	virtual void account_read(evutil_socket_t fd, short ev, void *ctx) = 0;

	virtual void account_timer(evutil_socket_t fd, short ev, void *ctx) = 0;
	virtual void router_timer(evutil_socket_t fd, short ev, void *ctx) = 0;
	virtual void status_timer(evutil_socket_t fd, short ev, void *ctx) = 0;
	
public:
	struct event_base *m_base;

	evutil_socket_t m_account_fd;
	struct event *m_account_timer;

	evutil_socket_t m_router_fd;
	struct event *m_router_timer;

	evutil_socket_t m_status_fd;
	struct event *m_status_timer;
	string m_status_ip;
	unsigned int m_status_port;
	bool m_status_enable;

	config_file_oper *m_config_file;
};

#endif