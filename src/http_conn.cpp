#include "http_conn.h"
#include "http_server.h"
#include "im_log.h"
#include <unistd.h>

http_conn::http_conn(evutil_socket_t fd, string ip, struct event *ev, void *arg)
{
	m_fd = fd;
	m_ip = ip;
	m_event = ev;
	m_http_server = new http_server;
	m_http_server->m_conn = this;
	m_arg = arg;
}

http_conn::~http_conn()
{
	free();
}

void http_conn::free()
{
	close(m_fd);
	m_ip.clear();
	if (NULL != m_event)
	{
		event_free(m_event);
		m_event = NULL;
	}
	if (NULL != m_http_server)
	{
		delete m_http_server;
		m_http_server = NULL;
	}	
	m_arg = NULL;
}