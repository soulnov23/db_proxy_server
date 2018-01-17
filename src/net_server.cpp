#include "net_server.h"
#include "config_file_oper.h"
#include "message.h"
#include "handler_router.h"
#include <arpa/inet.h>
#include "im_log.h"
#include "http_server.h"
#include <unistd.h>
#include "json_task.h"
#include "pb_task.h"

net_server *net_server::g_net_server = new net_server;

net_server::net_server()
{
	m_send_thread = new send_thread;
	deque<task *>().swap(m_response_queue);
}

net_server::~net_server()
{
	stop();
}

void net_server::start()
{
	if (0 != start_net_work())
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:start_net_work() failed", __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	if (0 != m_thread_pool.init())
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:m_thread_pool.init() failed", __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	if (!m_send_thread->create())
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:create sendthread failed", __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	do_http_listen();
	do_listen();
	if (0 != do_router_connect())
	{
		log(LOG_DEBUG, "Reconnect router......");
		do_router_re_connect();
	}
	/*
	if (0 != do_account_connect())
	{
		log(LOG_DEBUG, "Reconnect account......");
		do_account_re_connect();
	}
	*/
	do_dispatch();
}

void net_server::stop()
{
	stop_net_work();
	m_thread_pool.destory();
	if (NULL != m_send_thread)
	{
		delete m_send_thread;
		m_send_thread = NULL;
	}
}

net_server *net_server::get_instance()
{
	return g_net_server;
}

void net_server::account_read(evutil_socket_t fd, short ev, void *ctx)
{
	net_conn *conn = (net_conn*)ctx;
	bool ret = account_read_data(conn);
	if (ret)
	{
		account_handle_data(conn);
	}
}

bool net_server::account_read_data(net_conn *conn)
{
	while (true)
	{
		int ret = evbuffer_read(conn->m_evbuffer, conn->m_fd, JSON_BUFFER_SIZE);
		if (ret > 0)
		{
			return true;
		}
		if (ret == -1)
		{
			if (errno == EAGAIN)
			{
				log(LOG_DEBUG, "EAGAIN");
				return false;
			}
			else if (errno == EINTR)
			{
				log(LOG_DEBUG, "EINTR");
				continue;
			}
			else
			{
				log(LOG_DEBUG, "[TCP][abnormal]Close account ip:%s socket:%d", conn->m_ip.c_str(), conn->m_fd);
				if (0 != do_account_connect())
				{
					do_account_re_connect();
				}
				delete conn;
				return false;
			}
		}
		if (ret == 0)
		{
			log(LOG_DEBUG, "[TCP][normal]Close account ip:%s socket:%d", conn->m_ip.c_str(), conn->m_fd);
			if (0 != do_account_connect())
			{
				do_account_re_connect();
			}
			delete conn;
			return false;
		}
	}
}

void net_server::account_handle_data(net_conn *conn)
{
	while (true)
	{
		int length = account_handle_head(conn->m_evbuffer);
		if (length == -1)
		{
			return;
		}
		if (length > JSON_BUFFER_SIZE)
		{
			evbuffer_drain(conn->m_evbuffer, evbuffer_get_length(conn->m_evbuffer));
			continue;
		}
		char *msg = new char[length+1];
		evbuffer_remove(conn->m_evbuffer, msg, length+1);
		log(LOG_DEBUG, "json_data:%s", msg);
		task *ptask = new json_task(conn->m_fd, msg, length+1);
		if (ptask != NULL)
		{
			m_thread_pool.add_task(ptask);
		}
	}
}

int net_server::account_handle_head(struct evbuffer *input)
{
	int sum = 0;
	while (true)
	{
		struct evbuffer_iovec vec[1];
		if (evbuffer_peek(input, -1, NULL, vec, 1) < 1)
		{
			return -1;
		}
		for (unsigned int i = 0; i < vec->iov_len; i++)
		{
			char start = *((char*)(vec->iov_base) + i);
			if (start == '{')
			{
				evbuffer_drain(input, i);
				while (true)
				{
					struct evbuffer_iovec vec_temp[1];
					if (evbuffer_peek(input, -1, NULL, vec_temp, 1) < 1)
					{
						return -1;
					}
					for (unsigned int j = 0; j < vec_temp->iov_len; j++)
					{
						char end = *((char*)(vec_temp->iov_base) + j);
						if (end == '\0')
						{
							sum = sum + j;
							return sum;
						}
					}
					sum = sum + vec_temp->iov_len;
				}
			}
		}
		evbuffer_drain(input, vec->iov_len);
		continue;
	}
}

void net_server::http_read(evutil_socket_t fd, short ev, void *ctx)
{
	http_conn *conn = (http_conn*)ctx;
	while (true)
	{
		static char buffer[HTTP_BUFFER_SIZE];
		int ret = recv(fd, buffer, HTTP_BUFFER_SIZE, 0);
		if (ret > 0)
		{
			conn->m_http_server->parse(buffer, ret);
			if (ret < HTTP_BUFFER_SIZE)
			{
				return;
			}
			continue;
		}
		if (ret == -1)
		{
			if (errno == EAGAIN)
			{
				//log(LOG_DEBUG, "EAGAIN");
				return;
			}
			else if (errno == EINTR)
			{
				//log(LOG_DEBUG, "EINTR");
				continue;
			}
			else
			{
				log(LOG_DEBUG, "[HTTP][abnormal]Close connection ip:%s socket:%d", conn->m_ip.c_str(), fd);
				delete conn;
			}
			return;
		}
		if (ret == 0)
		{
			log(LOG_DEBUG, "[HTTP][normal]Close connection ip:%s socket:%d", conn->m_ip.c_str(), fd);
			delete conn;
			return;
		}
	}
}

void net_server::read(evutil_socket_t fd, short ev, void *ctx)
{
	net_conn *conn = (net_conn*)ctx;
	bool ret = read_data(conn);
	if (ret)
	{
		handle_data(conn);
	}
}

bool net_server::read_data(net_conn *conn)
{
	while (true)
	{		
		int ret = evbuffer_read(conn->m_evbuffer, conn->m_fd, TCP_BUFFER_SIZE);
		if (ret > 0)
		{
			return true;
		}
		if (ret == -1)
		{
			if (errno == EAGAIN)
			{
				log(LOG_DEBUG, "EAGAIN");
				return false;
			}
			else if (errno == EINTR)
			{
				log(LOG_DEBUG, "EINTR");
				continue;
			}
			else
			{
				if (m_router_fd == conn->m_fd)
				{
					log(LOG_DEBUG, "[TCP][abnormal]Close router ip:%s socket:%d", conn->m_ip.c_str(), conn->m_fd);
					if (0 != do_router_connect())
					{
						do_router_re_connect();
					}
				}
				else if (m_status_fd == conn->m_fd)
				{
					log(LOG_DEBUG, "[TCP][abnormal]Close status ip:%s socket:%d", conn->m_ip.c_str(), conn->m_fd);
					m_status_enable = false;
				}
				else
				{
					log(LOG_DEBUG, "[TCP][abnormal]Close connection ip:%s socket:%d", conn->m_ip.c_str(), conn->m_fd);
				}			
				delete conn;		
				return false;
			}
		}
		if (ret == 0)
		{
			if (m_router_fd == conn->m_fd)
			{
				log(LOG_DEBUG, "[TCP][normal]Close router ip:%s socket:%d", conn->m_ip.c_str(), conn->m_fd);
				if (0 != do_router_connect())
				{
					do_router_re_connect();
				}
			}
			else if (m_status_fd == conn->m_fd)
			{
				log(LOG_DEBUG, "[TCP][normal]Close status ip:%s socket:%d", conn->m_ip.c_str(), conn->m_fd);
				m_status_enable = false;
			}
			else
			{
				log(LOG_DEBUG, "[TCP][normal]Close connection ip:%s socket:%d", conn->m_ip.c_str(), conn->m_fd);
			}
			delete conn;
			return false;
		}
	}
}

void net_server::handle_data(net_conn *conn)
{
	while (true)
	{
		message *msg = new message;
		while (!msg->read_head(conn->m_evbuffer))
		{
			delete msg;
			msg = NULL;
			return;
		}
		int len = evbuffer_get_length(conn->m_evbuffer);
		uint16_t pb_length = msg->get_pb_length();
		if (len < pb_length + HEADER_LEN)
		{
			delete msg;
			msg = NULL;
			return;
		}
		else
		{
			if (pb_length + HEADER_LEN > TCP_BUFFER_SIZE)
			{
				evbuffer_drain(conn->m_evbuffer, pb_length + HEADER_LEN);
				delete msg;
				msg = NULL;
				continue;
			}
			msg->read_msg(conn->m_evbuffer);
			task *ptask = new pb_task(conn->m_fd, msg);
			if (ptask != NULL)
			{
				m_thread_pool.add_task(ptask);
			}
			if (len == pb_length + HEADER_LEN)
			{
				return;
			}
		}
	}
}

void net_server::router_timer(evutil_socket_t fd, short ev, void *ctx)
{
	log(LOG_DEBUG, "Reconnect router......");
	if (0 != do_router_connect())
	{
		do_router_re_connect();
	}
}

void net_server::account_timer(evutil_socket_t fd, short ev, void *ctx)
{
	log(LOG_DEBUG, "Reconnect account......");
	if (0 != do_account_connect())
	{
		do_account_re_connect();
	}
}

void net_server::status_timer(evutil_socket_t fd, short ev, void *ctx)
{
	if (!m_status_enable)
	{
		log(LOG_DEBUG, "Reconnect status......");
		if (0 != do_status_connect())
		{
			do_status_re_connect();
		}
	}
}

void net_server::signal_response()
{
	m_lock.lock();
	m_lock.signal();
	m_lock.unlock();
}

void net_server::add_response(task *ptask)
{
	m_lock.lock();
	m_response_queue.push_back(ptask);
	m_lock.signal();
	m_lock.unlock();
}

void net_server::send_response()
{
	m_lock.lock();
	if (!m_response_queue.empty())
	{
		auto ptask = m_response_queue.front();
		m_response_queue.pop_front();
		m_lock.unlock();
		ptask->send();
	}
	else
	{
		m_lock.wait();
		m_lock.unlock();
	}
}

void net_server::http_send(const char *buf, int len)
{
	do_http_connect(buf, len);
}

void net_server::status_send(const char *buf, int len)
{
	unsigned int pos = 0;
	do
	{
		int ret = send(m_status_fd, buf + pos, len - pos, 0);
		if (-1 == ret)
		{
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
			{
				log(LOG_DEBUG, "send error:%s", strerror(errno));
				ret = 0;
				continue;
			}
			else
			{
				break;
			}
		}
		if (0 == ret)
		{
			break;
		}
		pos += ret;
	} while (pos < len);
}

void net_server::account_send(const char *buf, int len)
{
	unsigned int pos = 0;
	do
	{
		int ret = send(m_account_fd, buf + pos, len - pos, 0);
		if (-1 == ret)
		{
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
			{
				log(LOG_DEBUG, "send error:%s", strerror(errno));
				ret = 0;
				continue;
			}
			else
			{
				break;
			}
		}
		if (0 == ret)
		{
			break;
		}
		pos += ret;
	} while (pos < len);
}

void net_server::insert_map(uint64_t key, task *ptask)
{
	m_session_lock.lock();
	m_login_session.insert(make_pair(key, ptask));
	m_session_lock.unlock();
}

task *net_server::find_map(uint64_t key)
{
	task *ptask = NULL;
	m_session_lock.lock();
	map<uint64_t, task*>::iterator it = m_login_session.find(key);
	if (it != m_login_session.end())
	{
		ptask = it->second;
	}
	m_session_lock.unlock();
	return ptask;
}

void net_server::eraser_map(uint64_t key)
{
	m_session_lock.lock();
	map<uint64_t, task*>::iterator it = m_login_session.find(key);
	if (it != m_login_session.end())
	{
		delete(it->second);
		m_login_session.erase(it);
	}
	m_session_lock.unlock();
}

send_thread::send_thread()
{
	m_flag = true;
}

send_thread::~send_thread()
{
	m_flag = false;
	net_server::get_instance()->signal_response();
	wait();
}

void send_thread::run()
{
	while (m_flag)
	{
		net_server::get_instance()->send_response();
	}
}
