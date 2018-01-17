#include "json_task.h"
#include "../jsoncpp/json/json.h"
using namespace Json;
#include "im_log.h"
#include "handler_account.h"

json_task::json_task(evutil_socket_t fd, char *msg, int length)
{
	m_fd = fd;
	m_msg = msg;
	m_length = length;
}

json_task::~json_task()
{
	if (m_msg)
	{
		delete m_msg;
		m_msg = NULL;
	}
}

void json_task::run(MYSQL *mysql, cache_conn *redis)
{
	Reader reader;
	Value value;
	do
	{
		if (!reader.parse(m_msg, value))
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d:json parse failed", __FILE__, __FUNCTION__, __LINE__);
			break;
		}
		if (value["protoType"].isNull())
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d:json head is null", __FILE__, __FUNCTION__, __LINE__);
			break;
		}
		if (string("app:connect:rsp") == value["protoType"].asString())
		{
			account_app(value);
		}
		if (string("company:userinfo:login:rsp") == value["protoType"].asString())
		{
			account_login(m_fd, value, mysql, redis);
		}
		if (string("all:company:userlist:rsp") == value["protoType"].asString())
		{
			account_init(value);
		}
	} while (false);
}

void json_task::send()
{
	unsigned int pos = 0;
	do
	{
		int ret = ::send(m_fd, m_msg + pos, m_length - pos, 0);
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
	} while (pos < m_length);
	delete this;
	//log(LOG_DEBUG, "Response send and length = %d", HEADER_LEN + ptask->m_msg->get_pb_length());
}