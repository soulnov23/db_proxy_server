#include "cache_conn.h"
#include "config_file_oper.h"
#include "im_log.h"

cache_conn::cache_conn()
{
	m_context = NULL;
}

cache_conn::~cache_conn()
{
	if (m_context)
	{
		redisFree(m_context);
		m_context = NULL;
	}
}

int cache_conn::init()
{
	if (m_context)
	{
		return 0;
	}
	config_file_oper config_file("server_config.conf");
	string server_ip = config_file.get_config_value("redis_ip");
	unsigned int server_port = atoi(config_file.get_config_value("redis_port"));
	struct timeval timeout = { 2, 0 };
	m_context = redisConnectWithTimeout(server_ip.c_str(), server_port, timeout);
	if ((NULL == m_context) || (m_context->err))
	{
		if (m_context)
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d:redisConnectWithTimeout( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, m_context->errstr);
			redisFree(m_context);
			m_context = NULL;
		}
		return 1;
	}
	unsigned int server_num = atoi(config_file.get_config_value("redis_num"));
	redisReply *reply = (redisReply*)redisCommand(m_context, "select %d", server_num);
	if (reply && (reply->type == REDIS_REPLY_STATUS) && (strncmp(reply->str, "OK", reply->len) == 0))
	{
		freeReplyObject(reply);
		return 0;
	}
	else
	{
		freeReplyObject(reply);
		log(LOG_ERROR, "[ERROR] %s:%s():%d:redisCommand( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, m_context->errstr);
		return 1;
	}
}

string cache_conn::set(string key, string value)
{
	string ret_value;
	if (init())
	{
		return ret_value;
	}
// 	const char *argv[3];
// 	size_t argvlen[3];
// 	argv[0] = "mset";
// 	argvlen[0] = 4;
// 	argv[1] = key.c_str();
// 	argvlen[1] = key.length();
// 	argv[2] = value.c_str();
// 	argvlen[2] = value.length();
// 	redisReply *reply = (redisReply *)redisCommandArgv(m_context, 3, argv, argvlen);
	redisReply *reply = (redisReply *)redisCommand(m_context, "set %s %b", key.c_str(), value.c_str(), value.length());
	if (!reply)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:redisCommand( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, m_context->errstr);
		redisFree(m_context);
		m_context = NULL;
		return ret_value;
	}
	ret_value.append(reply->str, reply->len);
	freeReplyObject(reply);
	return ret_value;
}

string cache_conn::get(string key)
{
	string ret_value;
	if (init())
	{
		return ret_value;
	}
	redisReply *reply = (redisReply *)redisCommand(m_context, "get %s", key.c_str());
	if (!reply)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:redisCommand( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, m_context->errstr);
		redisFree(m_context);
		m_context = NULL;
		return ret_value;
	}
	if (reply->type == REDIS_REPLY_STRING)
	{
		ret_value.append(reply->str, reply->len);
	}
	freeReplyObject(reply);
	return ret_value;
}
	
bool cache_conn::del(string key)
{
	if (init())
	{
		return false;
	}
	redisReply *reply = (redisReply *)redisCommand(m_context, "del %s", key.c_str());
	if (!reply)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:redisCommand( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, m_context->errstr);
		redisFree(m_context);
		m_context = NULL;
		return false;
	}
	bool retValue = true;
	if (reply->type != REDIS_REPLY_INTEGER)
	{
		retValue = false;
	}
	freeReplyObject(reply);
	return retValue;
}

