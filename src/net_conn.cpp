#include "net_conn.h"
#include "im_log.h"
#include <unistd.h>

net_conn::net_conn(evutil_socket_t fd, string ip, struct event *ev, void *arg)
{
	m_fd = fd;
	m_ip = ip;
	m_event = ev;
	m_evbuffer = evbuffer_new();
	m_arg = arg;
}

net_conn::~net_conn()
{
	free();
}

void net_conn::free()
{
	close(m_fd);
	m_ip.clear();
	if (NULL != m_event)
	{
		event_free(m_event);
		m_event = NULL;
	}
	if (NULL != m_evbuffer)
	{
		evbuffer_free(m_evbuffer);
		m_evbuffer = NULL;
	}
	m_arg = NULL;
}