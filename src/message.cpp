#include "message.h"
#include "im_log.h"
#include "event.h"

message::message()
{
	m_msg = NULL;
	memset(&m_msg_head, 0, 27);
}

message::~message()
{
	if (m_msg)
	{
		free(m_msg);
		m_msg = NULL;
	}
}

bool message::read_head(struct evbuffer *input)
{
	while (true)
	{
		unsigned int len = evbuffer_get_length(input);
		if (len < HEADER_LEN)
		{
			return false;
		}
		char msg_head[HEADER_LEN];
		evbuffer_copyout(input, msg_head, HEADER_LEN);
		const char *temp = msg_head;
		m_msg_head.s_tag = ntohs(*((uint16_t*)temp));
		temp += 2;
		m_msg_head.s_node = ntohl(*((uint32_t*)temp));
		temp += 4;
		m_msg_head.s_socket = ntohl(*((uint32_t*)temp));
		temp += 4;
		m_msg_head.s_router = *(temp);
		temp += 1;
		m_msg_head.c_tag = ntohs(*((uint16_t*)temp));
		temp += 2;
		m_msg_head.version = *(temp);
		temp += 1;
		m_msg_head.seq_num = ntohs(*((uint16_t*)temp));
		temp += 2;
		m_msg_head.service_id = ntohs(*((uint16_t*)temp));
		temp += 2;
		m_msg_head.cmd_id = ntohs(*((uint16_t*)temp));
		temp += 2;
		m_msg_head.company_id = ntohl(*((uint32_t*)temp));
		temp += 4;
		m_msg_head.pb_length = ntohs(*((uint16_t*)temp));
		temp += 2;
		m_msg_head.check_sum = ntohs(*((uint8_t*)temp));
		if (S_TAG_VALUE == m_msg_head.s_tag)
		{
			if (C_TAG_VALUE == m_msg_head.c_tag)
			{
				return true;
			}
			else
			{
				evbuffer_drain(input, 2);
			}
		}
		if (handle_head(input))
		{
			continue;
		}
		else
		{
			return false;
		}
	}
}


bool message::handle_head(struct evbuffer *input)
{
	while (true)
	{
		struct evbuffer_iovec vec[1];
		if (evbuffer_peek(input, -1, NULL, vec, 1) < 1)
		{
			return false;
		}
		for (unsigned int i = 0; i < vec->iov_len; i++)
		{
			uint8_t s_tag_7e = *((uint8_t*)(vec->iov_base) + i);
			if (s_tag_7e == S_TAG_7E)
			{
				evbuffer_drain(input, i);
				struct evbuffer_iovec vec_temp[1];
				if (evbuffer_peek(input, -1, NULL, vec_temp, 1) < 1)
				{
					return false;
				}
				uint8_t s_tag_aa;
				if (i == vec->iov_len - 1)
				{
					s_tag_aa = *((uint8_t*)(vec_temp->iov_base));
				}
				else
				{
					s_tag_aa = *((uint8_t*)(vec_temp->iov_base) + 1);
				}
				if (s_tag_aa == S_TAG_AA)
				{
					return true;
				}
				else
				{
					evbuffer_drain(input, 1);
					continue;
				}
			}
		}
		evbuffer_drain(input, vec->iov_len);
		continue;
	}
}

void message::read_msg(struct evbuffer *input)
{
	m_msg = (char*)realloc(m_msg, HEADER_LEN + m_msg_head.pb_length);
	evbuffer_remove(input, m_msg, HEADER_LEN + m_msg_head.pb_length);
}

uint32_t message::get_s_node()
{
	return m_msg_head.s_node;
}

uint32_t message::get_s_socket()
{
	return m_msg_head.s_socket;
}

uint8_t message::get_s_router()
{
	return m_msg_head.s_router;
}

uint16_t message::get_seq_num()
{
	return m_msg_head.seq_num;
}

uint16_t message::get_service_id()
{
	return m_msg_head.service_id;
}

uint16_t message::get_cmd_id()
{
	return m_msg_head.cmd_id;
}

uint32_t message::get_company_id()
{
	return m_msg_head.company_id;
}

uint16_t message::get_pb_length()
{
	return m_msg_head.pb_length;
}

uint8_t message::get_check_sum()
{
	return m_msg_head.check_sum;
}

char *message::get_pb_data()
{
	return m_msg + HEADER_LEN;
}

void message::set_s_node(uint32_t s_node)
{
	m_msg_head.s_node = s_node;
}

void message::set_s_socket(uint32_t s_socket)
{
	m_msg_head.s_socket = s_socket;
}

void message::set_s_router(uint8_t s_router)
{
	m_msg_head.s_router = s_router;
}

void message::set_seq_num(uint8_t seq_num)
{
	m_msg_head.seq_num = seq_num;
}

void message::set_service_id(uint16_t service_id)
{
	m_msg_head.service_id = service_id;
}

void message::set_cmd_id(uint16_t cmd_id)
{
	m_msg_head.cmd_id = cmd_id;
}

void message::set_company_id(uint32_t company_id)
{
	m_msg_head.company_id = company_id;
}

void message::set_pb_length(uint16_t pb_length)
{
	m_msg_head.pb_length = pb_length;
}

void message::write_msg(const google::protobuf::MessageLite *pb_data)
{
	m_msg = (char*)realloc(m_msg, HEADER_LEN + m_msg_head.pb_length);
	char *temp = m_msg;
	*((uint16_t*)temp) = htons(S_TAG_VALUE);
	temp += 2;
	*((uint32_t*)temp) = htonl(m_msg_head.s_node);
	temp += 4;
	*((uint32_t*)temp) = htonl(m_msg_head.s_socket);
	temp += 4;
	*(temp) = htons(m_msg_head.s_router);
	temp += 1;
	*((uint16_t*)temp) = htons(C_TAG_VALUE);
	temp += 2;
	*(temp) = htons(VERSION);
	temp += 1;
	*((uint16_t*)temp) = htons(m_msg_head.seq_num);
	temp += 2;
	*((uint16_t*)temp) = htons(m_msg_head.service_id);
	temp += 2;
	*((uint16_t*)temp) = htons(m_msg_head.cmd_id);
	temp += 2;
	*((uint32_t*)temp) = htonl(m_msg_head.company_id);
	temp += 4;
	*((uint16_t*)temp) = htons(m_msg_head.pb_length);
	if (!pb_data->SerializePartialToArray(m_msg + HEADER_LEN, pb_data->ByteSize()))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d", __FILE__, __FUNCTION__, __LINE__);
		return;
	}
	char c = temp[0] ^ temp[1];
	for (int i = 0; i < m_msg_head.pb_length; i++)
	{
		c ^= m_msg[HEADER_LEN + i];
	}
	temp += 2;
	*(temp) = c;
}

char *message::get_data()
{
	return m_msg;
}