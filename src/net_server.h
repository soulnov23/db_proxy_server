#ifndef _NET_SERVER_H_
#define _NET_SERVER_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：net_server.h
 *描		述：网络操作类
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2016-06-27 14:00
 *修改日期：2016-06-27
*******************************************************************************************/
#include "net_work.h"
#include "thread_pool.h"
#include <deque>
#include <map>
using namespace std;

#define TCP_BUFFER_SIZE 8*1024
#define JSON_BUFFER_SIZE 16*1024
#define HTTP_BUFFER_SIZE 16*1024

class send_thread : public base_thread
{
public:
	send_thread();
	~send_thread();

	void run();

private:
	bool m_flag;
};

class net_server : public net_work
{ 
public:
	net_server();
	~net_server();

	void start();
	void stop();

	static net_server *get_instance();

	void add_response(task *ptask);
	void send_response();
	void signal_response();

	void http_send(const char *buf, int len);
	void status_send(const char *buf, int len);
	void account_send(const char *buf, int len);

	void insert_map(uint64_t key, task *ptask);
	task *find_map(uint64_t key);
	void eraser_map(uint64_t key);

private:
	virtual void account_read(evutil_socket_t fd, short ev, void *ctx);
	bool account_read_data(net_conn *conn);
	void account_handle_data(net_conn *conn);
	int account_handle_head(struct evbuffer *input);
	virtual void http_read(evutil_socket_t fd, short ev, void *ctx);
	virtual void read(evutil_socket_t fd, short ev, void *ctx);
	bool read_data(net_conn *conn);
	void handle_data(net_conn *conn);

	virtual void account_timer(evutil_socket_t fd, short ev, void *ctx);
	virtual void router_timer(evutil_socket_t fd, short ev, void *ctx);
	virtual void status_timer(evutil_socket_t fd, short ev, void *ctx);

public:
	thread_pool m_thread_pool;

private:
	send_thread *m_send_thread;

	deque<task *> m_response_queue;
	thread_lock m_lock;

	map<uint64_t, task*> m_login_session;
	thread_lock m_session_lock;

	static net_server *g_net_server;
};

#endif
