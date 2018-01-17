#include "net_work.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include "im_log.h"
#include "message.h"
#include "../protobuf/IM.BaseDefine.pb.h"
using namespace IM::BaseDefine;
#include "../protobuf/IM.Router.pb.h"
#include "../json/json.h"
#include "md5.h"
#include <time.h>

net_work::net_work()
{
#ifdef WIN32
	WSADATA WSAData;
	if (0 != WSAStartup(MAKEWORD(2, 2), &WSAData))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:WSAStartup( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		return;
	}
	/*
	if (-1 == evthread_use_windows_threads())
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:evthread_use_windows_threads( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		return;
	}
	*/
#else
	/*
	if (-1 == evthread_use_pthreads())
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:evthread_use_pthreads( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		return;
	}
	*/
#endif // WIN32
	m_base = NULL;
	m_account_timer = NULL;
	m_router_timer = NULL;
	m_status_timer = NULL;
	m_status_ip.clear();
	m_status_port = -1;
	m_status_enable = false;
	m_config_file = new config_file_oper("server_config.conf");
}

net_work::~net_work()
{
	stop_net_work();
}

int net_work::start_net_work()
{
	signal(SIGPIPE, signal_handler_t);
	m_base = event_base_new();
	if (NULL == m_base)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:event_base_new( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	return 0;
}

void net_work::stop_net_work()
{
	if (NULL != m_config_file)
	{
		delete m_config_file;
		m_config_file = NULL;
	}
	if (NULL != m_account_timer)
	{
		event_free(m_account_timer);
		m_account_timer = NULL;
	}
	if (NULL != m_router_timer)
	{
		event_free(m_router_timer);
		m_router_timer = NULL;
	}
	if (NULL != m_status_timer)
	{
		event_free(m_status_timer);
		m_status_timer = NULL;
	}
	if (NULL != m_base)
	{
		struct timeval delay = { 2, 0 };
		event_base_loopexit(m_base, &delay);
		event_base_free(m_base);
		m_base = NULL;
	}	
#ifdef WIN32
	WSACleanup();
#endif // WIN32
}

void net_work::do_http_listen()
{
	int port = atoi(m_config_file->get_config_value("http_listen_port"));
	struct sockaddr_in http_addr;
	http_addr.sin_family = AF_INET;
	http_addr.sin_port = htons(port);
	http_addr.sin_addr.s_addr = INADDR_ANY;
	evutil_socket_t listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	evutil_make_socket_nonblocking(listener);
	int optval = 1;
	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (-1 == bind(listener, (struct sockaddr*)&http_addr, sizeof(http_addr)))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:bind():%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		close(listener);
		return;
	}
	if (-1 == listen(listener, 128))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:listen():%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		close(listener);
		return;
	}
	struct event *ev = event_new(m_base, listener, EV_READ | EV_PERSIST, do_http_accept, this);
	event_add(ev, NULL);
}

void net_work::do_listen()
{
	int port = atoi(m_config_file->get_config_value("listen_port"));
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	evutil_socket_t listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	evutil_make_socket_nonblocking(listener);
	int optval = 1;
	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (-1 == bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr)))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:bind():%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		close(listener);
		return;
	}
	if (-1 == listen(listener, 128))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:listen():%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		close(listener);
		return;
	}
	struct event *ev = event_new(m_base, listener, EV_READ | EV_PERSIST, do_accept, this);
	event_add(ev, NULL);
}

void net_work::do_http_accept(evutil_socket_t fd, short ev, void *ctx)
{
	while (true)
	{
		struct sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		//getpeername(fd, (struct sockaddr *)&addr, &addr_len);
		evutil_socket_t _fd = accept(fd, (struct sockaddr *)&addr, &addr_len);
		if (_fd == -1)
		{
			break;
		}
		log(LOG_DEBUG, "(HTTP)New accept ip:%s socket:%d", inet_ntoa(addr.sin_addr), _fd);
		evutil_make_socket_nonblocking(_fd);
		int optval = 1;
		setsockopt(_fd, SOL_SOCKET, TCP_NODELAY, &optval, sizeof(optval));
		struct event *_ev = new struct event;
		if (NULL == _ev)
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d:new struct event failed:%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
			continue;
		}
		http_conn *conn = new http_conn(_fd, inet_ntoa(addr.sin_addr), _ev, ctx);
		event_set(_ev, _fd, EV_READ | EV_PERSIST, do_http_read, conn);
		net_work *pthis = (net_work*)ctx;
		event_base_set(pthis->m_base, _ev);
		event_add(_ev, NULL);
	}
}

void net_work::do_accept(evutil_socket_t fd, short ev, void *ctx)
{
	while (true)
	{
		struct sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		//getpeername(fd, (struct sockaddr *)&addr, &addr_len);
		evutil_socket_t _fd = accept(fd, (struct sockaddr *)&addr, &addr_len);
		if (_fd == -1)
		{
			break;
		}
		log(LOG_DEBUG, "(TCP)New accept ip:%s socket:%d", inet_ntoa(addr.sin_addr), _fd);
		evutil_make_socket_nonblocking(_fd);
		int optval = 1;
		setsockopt(_fd, SOL_SOCKET, TCP_NODELAY, &optval, sizeof(optval));
		/*
		//开启keepalive属性
		setsockopt(_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
		optval = 60;
		//应用程序开启60s后开始进行tcp探测
		setsockopt(_fd, SOL_TCP, TCP_KEEPIDLE, &optval, sizeof(optval));
		//tcp探测发包间隔60s
		setsockopt(_fd, SOL_TCP, TCP_KEEPINTVL, &optval, sizeof(optval));
		optval = 3;
		//尝试探测的次数为3
		setsockopt(_fd, SOL_TCP, TCP_KEEPCNT, &optval, sizeof(optval));
		*/

		struct event *_ev = new struct event;
		if (NULL == _ev)
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d:new struct event failed:%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
			continue;
		}
		net_conn *conn = new net_conn(_fd, inet_ntoa(addr.sin_addr), _ev, ctx);
		event_set(_ev, _fd, EV_READ | EV_PERSIST, do_read, conn);
		net_work *pthis = (net_work*)ctx;
		event_base_set(pthis->m_base, _ev);
		event_add(_ev, NULL);
	}
}

int net_work::do_http_connect(const char *buf, int len)
{
	char *ip = m_config_file->get_config_value("http_ip");
	int port = atoi(m_config_file->get_config_value("http_port"));
	struct sockaddr_in http_addr;
	http_addr.sin_family = AF_INET;
	http_addr.sin_port = htons(port);
	http_addr.sin_addr.s_addr = inet_addr(ip);
	evutil_socket_t fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == fd)
	{
		return 1;
	}
	int optval = 1;
	setsockopt(fd, SOL_SOCKET, TCP_NODELAY, &optval, sizeof(optval));
	if (-1 == connect(fd, (struct sockaddr*)&http_addr, sizeof(http_addr)))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:connect( ) failed%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	evutil_make_socket_nonblocking(fd);

	struct event *ev = new struct event;
	if (NULL == ev)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:new struct event failed:%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	http_conn *conn = new http_conn(fd, ip, ev, this);
	event_set(ev, fd, EV_READ | EV_PERSIST, do_http_read, conn);
	event_base_set(m_base, ev);
	event_add(ev, NULL);

	unsigned int pos = 0;
	do
	{
		int ret = send(fd, buf+pos, len-pos, 0);
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
	return 0;
}

int net_work::do_account_connect()
{
	char *ip = m_config_file->get_config_value("account_ip");
	int port = atoi(m_config_file->get_config_value("account_port"));
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	m_account_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == m_account_fd)
	{
		return 1;
	}
	int optval = 1;
	setsockopt(m_account_fd, SOL_SOCKET, TCP_NODELAY, &optval, sizeof(optval));
	if (-1 == connect(m_account_fd, (struct sockaddr*)&addr, sizeof(addr)))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:connect( ) failed%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	evutil_make_socket_nonblocking(m_account_fd);
	log(LOG_DEBUG, "Connect account ip:%s socket:%d success", ip, m_account_fd);

	struct event *ev = new struct event;
	if (NULL == ev)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:new struct event failed:%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	net_conn *conn = new net_conn(m_account_fd, ip, ev, this);
	event_set(ev, m_account_fd, EV_READ | EV_PERSIST, do_account_read, conn);
	event_base_set(m_base, ev);
	event_add(ev, NULL);
	/*
	Json::Value json_data;
	json_data["protoType"] = "all:company:userlist:req";
	Json::FastWriter writer;
	string json = writer.write(json_data);
	unsigned int pos = 0;
	do
	{
		int ret = send(m_account_fd, json.c_str() + pos, json.length() - pos, 0);
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
	} while (pos < json.length());
	*/
	Json::Value json_app;
	json_app["msgid"] = "2017";
	json_app["protoType"] = "app:connect:req";
	json_app["clienttype"] = "app";
	json_app["appid"] = 10000001;
	string appkey_temp = to_string(time(NULL) / 3600) + "app1key2017" + "$dr2017lf^#";
	MD5 md5_appkey(appkey_temp);
	json_app["appkey"] = md5_appkey.md5();
	Json::FastWriter writer;
	string json = writer.write(json_app);
	json.append(1, '\0');
	unsigned int pos = 0;
	do
	{
		int ret = send(m_account_fd, json.c_str() + pos, json.length() - pos, 0);
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
	} while (pos < json.length());
	return 0;
}

int net_work::do_router_connect()
{
	char *ip = m_config_file->get_config_value("router_ip");
	int port = atoi(m_config_file->get_config_value("router_port"));
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	m_router_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == m_router_fd)
	{
		return 1;
	}
	int optval = 1;
	setsockopt(m_router_fd, SOL_SOCKET, TCP_NODELAY, &optval, sizeof(optval));
	if (-1 == connect(m_router_fd, (struct sockaddr*)&addr, sizeof(addr)))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:connect( ) failed%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	evutil_make_socket_nonblocking(m_router_fd);
	log(LOG_DEBUG, "Connect router ip:%s socket:%d success", ip, m_router_fd);

	struct event *ev = new struct event;
	if (NULL == ev)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:new struct event failed:%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	net_conn *conn = new net_conn(m_router_fd, ip, ev, this);
	event_set(ev, m_router_fd, EV_READ | EV_PERSIST, do_read, conn);
	event_base_set(m_base, ev);
	event_add(ev, NULL);	

	IM::Server::RegisterServerReq msg_data;
	IM::Server::ServerStatus *server_status = msg_data.mutable_current_server();
	unsigned int node_id = atoi(m_config_file->get_config_value("node_id"));
	string server_ip = m_config_file->get_config_value("db_server_ip");
	unsigned int server_port = atoi(m_config_file->get_config_value("listen_port"));
	server_status->set_node_id(node_id);
	server_status->add_service_type(SID_DB_PROXY);
	server_status->set_server_ip(server_ip.c_str());
	server_status->set_server_port(server_port);
	server_status->set_is_enable(true);
	message msg;
	msg.set_service_id(SID_ROUTER);
	msg.set_cmd_id(CID_ROUTER_CLIENT_REGISTER_REQ);
	msg.set_pb_length(msg_data.ByteSize());
	msg.write_msg(&msg_data);
	int ret = send(m_router_fd, msg.get_data(), HEADER_LEN + msg.get_pb_length(), 0);
	if (0 < ret)
	{
		//log(LOG_DEBUG, "Register router success and length = %d", HEADER_LEN + msg.get_pb_length());
	}
	else if (-1 == ret)
	{
		log(LOG_ERROR, "[ERROR] Register router failed", __FILE__, __FUNCTION__, __LINE__);
	}
	return 0;
}

int net_work::do_status_connect()
{
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_status_port);
	addr.sin_addr.s_addr = inet_addr(m_status_ip.c_str());
	m_status_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (-1 == m_status_fd)
	{
		return 1;
	}
	int optval = 1;
	setsockopt(m_status_fd, SOL_SOCKET, TCP_NODELAY, &optval, sizeof(optval));
	if (-1 == connect(m_status_fd, (struct sockaddr*)&addr, sizeof(addr)))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:connect( ) failed%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	evutil_make_socket_nonblocking(m_status_fd);
	log(LOG_DEBUG, "Connect status ip:%s socket:%d success", m_status_ip.c_str(), m_status_fd);
	m_status_enable = true;

	struct event *ev = new struct event;
	if (NULL == ev)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:new struct event failed:%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		return 1;
	}
	net_conn *conn = new net_conn(m_status_fd, m_status_ip.c_str(), ev, this);
	event_set(ev, m_status_fd, EV_READ | EV_PERSIST, do_read, conn);
	event_base_set(m_base, ev);
	event_add(ev, NULL);
	return 0;
}

void net_work::do_account_re_connect()
{
	if (NULL == m_account_timer)
	{
		m_account_timer = evtimer_new(m_base, do_account_timer, this);
	}
	struct timeval tv;
	tv.tv_sec = 30;
	tv.tv_usec = 0;
	evtimer_add(m_account_timer, &tv);
}

void net_work::do_router_re_connect()
{
	if (NULL == m_router_timer)
	{
		m_router_timer = evtimer_new(m_base, do_router_timer, this);
	}
	struct timeval tv;
	tv.tv_sec = 30;
	tv.tv_usec = 0;
	evtimer_add(m_router_timer, &tv);
}

void net_work::do_status_re_connect()
{
	if (NULL == m_status_timer)
	{
		m_status_timer = evtimer_new(m_base, do_status_timer, this);
	}
	struct timeval tv;
	tv.tv_sec = 30;
	tv.tv_usec = 0;
	evtimer_add(m_status_timer, &tv);
}

void net_work::do_dispatch()
{
	event_base_dispatch(m_base);
}

void net_work::do_http_read(evutil_socket_t fd, short ev, void *ctx)
{
	http_conn *conn = (http_conn*)ctx;
	if (conn)
	{
		net_work *pthis = (net_work*)(conn->m_arg);
		pthis->http_read(fd, ev, ctx);
	}
}

void net_work::do_read(evutil_socket_t fd, short ev, void *ctx)
{
	net_conn *conn = (net_conn*)ctx;
	if (conn)
	{
		net_work *pthis = (net_work*)(conn->m_arg);
		pthis->read(fd, ev, ctx);
	}
}

void net_work::do_account_read(evutil_socket_t fd, short ev, void *ctx)
{
	net_conn *conn = (net_conn*)ctx;
	if (conn)
	{
		net_work *pthis = (net_work*)(conn->m_arg);
		pthis->account_read(fd, ev, ctx);
	}
}

void net_work::do_account_timer(evutil_socket_t fd, short ev, void *ctx)
{
	net_work *pthis = (net_work*)ctx;
	if (pthis)
	{
		pthis->account_timer(fd, ev, ctx);
	}
}

void net_work::do_router_timer(evutil_socket_t fd, short ev, void *ctx)
{
	net_work *pthis = (net_work*)ctx;
	if (pthis)
	{
		pthis->router_timer(fd, ev, ctx);
	}
}

void net_work::do_status_timer(evutil_socket_t fd, short ev, void *ctx)
{
	net_work *pthis = (net_work*)ctx;
	if (pthis)
	{
		pthis->status_timer(fd, ev, ctx);
	}
}

void net_work::signal_handler_t(int signum)
{
	if (signum == SIGPIPE)
	{
		log(LOG_DEBUG, "Recv SIGPIPE signal");
	}
}

void net_work::set_status_ip(string ip)
{
	m_status_ip = ip;
}

void net_work::set_status_port(unsigned int port)
{
	m_status_port = port;
}
