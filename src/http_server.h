#ifndef _HTTP_MSG_H_
#define _HTTP_MSG_H_

#include "../http_parser/http_parser.h"
#include <string>
using namespace std;

class http_conn;

class http_server
{
public:
	http_server();
	~http_server();

	bool parse(const char *buf, size_t len);

public:
	static int message_begin_cb(http_parser *parser);
	static int url_cb(http_parser *parser, const char *at, size_t length);
	static int status_cb(http_parser *parser, const char *at, size_t length);
	//static int header_field_cb(http_parser *parser, const char *at, size_t length);
	//static int header_value_cb(http_parser *parser, const char *at, size_t length);
	static int headers_complete_cb(http_parser *parser);
	static int body_cb(http_parser *parser, const char *at, size_t length);
	static int message_complete_cb(http_parser *parser);
	//static int chunk_header_cb(http_parser *parser);
	//static int chunk_complete_cb(http_parser *parser);

private:
	void free();

private:
	http_parser *m_parser;
	http_parser_settings m_settings;

	//请求行
	string m_http_method;//请求方法Method
	string m_http_url;//Request-URI
	string m_http_version;//HTTP-Version
	//CRLF \r\n

	//请求报头
	//map<string, string> m_http_header;
	//string m_http_header_data;
	//\r\n\r\n

	//请求正文POST
	string m_http_body;

	
	//响应行
	//string m_http_version;//HTTP-Version "HTTP/1.1"
	int m_http_code;//状态代码 200
	string m_http_code_desc;//状态描述 "OK"
	//CRLF \r\n

	//响应报头
	//map<string, string> m_http_header;
	//string m_http_header_data;
	//\r\n\r\n

	//响应正文POST
	//string m_http_body;

public:
	http_conn *m_conn;
};

#endif