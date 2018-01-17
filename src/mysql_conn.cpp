#include "mysql_conn.h"
#include "config_file_oper.h"
#include "im_log.h"

mysql_conn::mysql_conn()
{
	m_mysql = mysql_init(NULL);
}

mysql_conn::~mysql_conn()
{

}

int mysql_conn::init()
{
	int ret = 0;
	int value = true;
	mysql_options(m_mysql, MYSQL_OPT_RECONNECT, (char *)&value);//设置断线重连
	mysql_options(m_mysql, MYSQL_SET_CHARSET_NAME, "utf8");//设置字符集
	config_file_oper config_file("server_config.conf");
	string db_server_ip = config_file.get_config_value("db_server_ip");
	string user = config_file.get_config_value("user");
	string password = config_file.get_config_value("password");
	string db_name = config_file.get_config_value("db_name");
	unsigned int db_server_port = atoi(config_file.get_config_value("db_server_port"));
	MYSQL *status = mysql_real_connect(m_mysql, db_server_ip.c_str(), user.c_str(), password.c_str(), db_name.c_str(), db_server_port, NULL, CLIENT_MULTI_STATEMENTS);
	if (NULL == status)
	{
		ret = 1;
		mysql_close(m_mysql);
		m_mysql = NULL;
		log(LOG_ERROR, "[ERROR] %s:%s():%d:%s", __FILE__, __FUNCTION__, __LINE__, mysql_error(m_mysql));
	}
	return ret;
}

MYSQL *mysql_conn::get_mysql()
{
	return m_mysql;
}

void mysql_conn::free()
{
	if (NULL != m_mysql)
	{
		mysql_close(m_mysql);
		m_mysql = NULL;
	}
}
