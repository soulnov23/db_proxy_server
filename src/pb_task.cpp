#include "pb_task.h"
#include "handler_map.h"
#include "im_log.h"

pb_task::pb_task(evutil_socket_t fd, message *msg)
{
	m_fd = fd;
	m_msg = msg;
}

pb_task::~pb_task()
{
	if (m_msg)
	{
		delete m_msg;
		m_msg = NULL;
	}
}

void *pb_task::get_msg()
{
	return m_msg;
}

evutil_socket_t pb_task::get_fd()
{
	return m_fd;
}

void pb_task::run(MYSQL *mysql, cache_conn *redis)
{
	msg_handler_t msg_handler = handler_map::get_instance()->get_handler(m_msg->get_cmd_id());
	if (msg_handler != NULL)
	{
		msg_handler(this, mysql, redis);
	}
	else
	{
		log(LOG_ERROR, "[ERROR] (TCP)Unregistered package");
	}
}

void pb_task::send()
{
	unsigned int pos = 0;
	do
	{
		int ret = ::send(m_fd, m_msg->get_data() + pos, HEADER_LEN + m_msg->get_pb_length() - pos, 0);
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
	} while (pos < (HEADER_LEN + m_msg->get_pb_length()));
	delete this;
	//log(LOG_DEBUG, "Response send and length = %d", HEADER_LEN + ptask->m_msg->get_pb_length());
}