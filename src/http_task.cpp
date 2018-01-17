#include "http_task.h"
#include "handler_http.h"
#include "../jsoncpp/json/json.h"
using namespace Json;
#include "im_log.h"

http_task::http_task(bool flag, evutil_socket_t fd, string *msg)
{
	m_flag = flag;
	m_fd = fd;
	m_msg = msg;
}

http_task::~http_task()
{
	if (m_msg)
	{
		delete m_msg;
		m_msg = NULL;
	}
}

void http_task::run(MYSQL *mysql, cache_conn *redis)
{
	if (m_flag == false)
	{
		return;
	}
	Reader reader;
	Value value;
	bool is_fail = true;
	do
	{
		if (!reader.parse(m_msg->data(), value))
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d:json parse failed", __FILE__, __FUNCTION__, __LINE__);
			break;
		}
		if (value["protocol_type"].isNull() || value["msgid"].isNull())
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d:json head is null", __FILE__, __FUNCTION__, __LINE__);
			break;
		}
		if (string("admin_login_req") == value["protocol_type"].asString())
		{
			http_login(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("query_company_list_req") == value["protocol_type"].asString())
		{
			http_get_company_list(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("query_company_user_list_req") == value["protocol_type"].asString())
		{
			http_get_company_user_list(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("get_company_organization_req") == value["protocol_type"].asString())
		{
			http_get_company_organization(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("get_organization_req") == value["protocol_type"].asString())
		{
			http_get_organization(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("get_all_node_member_req") == value["protocol_type"].asString())
		{
			http_get_all_node_member(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("get_all_member_req") == value["protocol_type"].asString())
		{
			http_get_all_member(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("get_node_member_req") == value["protocol_type"].asString())
		{
			http_get_node_member(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("get_user_info_req") == value["protocol_type"].asString())
		{
			http_get_user_info(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("add_company_req") == value["protocol_type"].asString())
		{
			http_add_company(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("modify_company_info_req") == value["protocol_type"].asString())
		{
			http_modify_company_info(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("add_department_req") == value["protocol_type"].asString())
		{
			http_add_department(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("modify_department_info_req") == value["protocol_type"].asString())
		{
			http_modify_department_info(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("del_department_req") == value["protocol_type"].asString())
		{
			http_del_department(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("add_user_req") == value["protocol_type"].asString())
		{
			http_add_user(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("modify_user_info_req") == value["protocol_type"].asString())
		{
			http_modify_user_info(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("user_title_add_req") == value["protocol_type"].asString())
		{
			http_add_user_title(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("user_title_modify_req") == value["protocol_type"].asString())
		{
			http_modify_user_title(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("user_title_del_req") == value["protocol_type"].asString())
		{
			http_del_user_title(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("del_user_req") == value["protocol_type"].asString())
		{
			http_del_user(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if(string("mov_node_req") == value["protocol_type"].asString())
		{
			http_mov_node(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("get_msg_statistics_req") == value["protocol_type"].asString())
		{
			http_msg_statistics_rsp(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("get_login_statistics_req") == value["protocol_type"].asString())
		{
			http_login_statistics_rsp(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("add_role_req") == value["protocol_type"].asString())
		{
			http_add_role(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("modify_role_attribute_req") == value["protocol_type"].asString())
		{
			http_modify_role_attribute(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("query_role_list_req") == value["protocol_type"].asString())
		{
			http_query_role_list(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("query_role_authority_req") == value["protocol_type"].asString())
		{
			http_query_role_authority(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("add_role_authority_req") == value["protocol_type"].asString())
		{
			http_add_role_authority(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("modify_role_authority_req") == value["protocol_type"].asString())
		{
			http_modify_role_authority(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("del_role_authority_req") == value["protocol_type"].asString())
		{
			http_del_role_authority(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("del_role_req") == value["protocol_type"].asString())
		{
			http_del_role(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("query_role_user_list_req") == value["protocol_type"].asString())
		{
			http_query_role_user_list(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("add_user_role_req") == value["protocol_type"].asString())
		{
			http_add_user_role(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("http_query_user_role") == value["protocol_type"].asString())
		{
			http_query_user_role(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else if (string("http_del_user_role") == value["protocol_type"].asString())
		{
			http_del_user_role(m_fd, value["msgid"].asString(), value["body"], mysql, redis);
		}
		else
		{
			log(LOG_ERROR, "[ERROR] (HTTP)Unregistered package");
			break;
		}
		is_fail = false;
	} while (false);
	if (is_fail)
	{
		string ret("fail");
		http_response(m_fd, ret);
	}
}

void http_task::send()
{
	unsigned int pos = 0;
	do
	{
		int ret = ::send(m_fd, m_msg->c_str() + pos, m_msg->length() - pos, 0);
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
	} while (pos < m_msg->length());
	delete this;
	//log(LOG_DEBUG, "Response send and length = %d", HEADER_LEN + ptask->m_msg->get_pb_length());
}