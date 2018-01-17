#include "http_server.h"
#include "net_server.h"
#include "im_log.h"
#include "http_conn.h"
#include "http_task.h"

http_server::http_server()
{
	m_parser = new http_parser;
	if (NULL != m_parser)
	{
		http_parser_init(m_parser, HTTP_BOTH);
		m_parser->data = this;
	}
	http_parser_settings_init(&m_settings);
	m_settings.on_message_begin = message_begin_cb;
	m_settings.on_url = url_cb;
	m_settings.on_status = status_cb;
//	m_settings.on_header_field = header_field_cb;
//	m_settings.on_header_value = header_value_cb;
	m_settings.on_headers_complete = headers_complete_cb;
	m_settings.on_body = body_cb;
	m_settings.on_message_complete = message_complete_cb;
//	m_settings.on_chunk_header = chunk_header_cb;
//	m_settings.on_chunk_complete = chunk_complete_cb;
	free();
}

http_server::~http_server()
{
	if (NULL != m_parser)
	{
		delete m_parser;
		m_parser = NULL;
	}
	http_parser_settings_init(&m_settings);
	free();
}

bool http_server::parse(const char *buf, size_t len)
{
	size_t ret = http_parser_execute(m_parser, &m_settings, buf, len);
	if (len == ret)
	{
		return true;
	}
	else
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:%s:%s", __FILE__, __FUNCTION__, __LINE__, http_errno_name(HTTP_PARSER_ERRNO(m_parser)), http_errno_description(HTTP_PARSER_ERRNO(m_parser)));
		return false;
	}
}

void http_server::free()
{
	m_http_method.clear();
	m_http_url.clear();
	m_http_version.clear();
	m_http_code = 0;
	m_http_code_desc.clear();
	//map<string, string>().swap(m_http_header);
	//m_http_header_data.clear();
	m_http_body.clear();
}

int http_server::message_begin_cb(http_parser *parser)
{
	http_server *pthis = (http_server *)parser->data;
	pthis->free();
	return 0;
}

int http_server::url_cb(http_parser *parser, const char *at, size_t length)
{
	http_server *pthis = (http_server *)parser->data;
	pthis->m_http_url.append(at, length);
	return 0;
}

int http_server::status_cb(http_parser *parser, const char *at, size_t length)
{
	http_server *pthis = (http_server *)parser->data;
	pthis->m_http_code_desc.append(at, length);
	return 0;
}

/*
int http_server::header_field_cb(http_parser *parser, const char *at, size_t length)
{
	return 0;
}

int http_server::header_value_cb(http_parser *parser, const char *at, size_t length)
{
	return 0;
}
*/

int http_server::headers_complete_cb(http_parser *parser)
{
	http_server *pthis = (http_server *)parser->data;
	pthis->m_http_version = string("HTTP/") + to_string(parser->http_major) + string(".") + to_string(parser->http_minor);
	if (parser->type == HTTP_REQUEST)
	{
		if (parser->method == HTTP_GET)
		{
			pthis->m_http_method = "GET";
		}
		else if (parser->method == HTTP_POST)
		{
			pthis->m_http_method = "POST";
		}
		log(LOG_DEBUG, "%s %s %s", pthis->m_http_method.c_str(), pthis->m_http_url.c_str(), pthis->m_http_version.c_str());
	}
	else if (parser->type == HTTP_RESPONSE)
	{
		pthis->m_http_code = parser->status_code;
		log(LOG_DEBUG, "%s %d %s", pthis->m_http_version.c_str(), pthis->m_http_code, pthis->m_http_code_desc.c_str());
	}
	return 0;
}

int http_server::body_cb(http_parser *parser, const char *at, size_t length)
{
	http_server *pthis = (http_server *)parser->data;
	pthis->m_http_body.append(at, length);
	return 0;
}

int http_server::message_complete_cb(http_parser *parser)
{
	http_server *pthis = (http_server *)parser->data;
	if (!pthis->m_http_body.empty())
	{
		log(LOG_DEBUG, "[http_recv fd=%u]%s", pthis->m_conn->m_fd,pthis->m_http_body.c_str());
		string *string_data = new string;
		*string_data = pthis->m_http_body;
		if (parser->type == HTTP_REQUEST)
		{
			task *ptask = new http_task(true, pthis->m_conn->m_fd, string_data);
			net_server::get_instance()->m_thread_pool.add_task(ptask);
		}
		else if (parser->type == HTTP_RESPONSE)
		{
			task *ptask = new http_task(false, pthis->m_conn->m_fd, string_data);
			net_server::get_instance()->m_thread_pool.add_task(ptask);
		}
	}
	if (parser->type == HTTP_RESPONSE)
	{
		delete pthis->m_conn;
	}
	pthis->free();
	return 0;
}

// int http_server::chunk_header_cb(http_parser *parser)
// {
// 	return 0;
// }
// 
// int http_server::chunk_complete_cb(http_parser *parser)
// {
// 	return 0;
// }
