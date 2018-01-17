#include "pre_statement.h"
#include <string.h>
#include "im_log.h"

pre_statement::pre_statement()
{
	m_mysql = NULL;
	m_stmt = NULL;
	m_param_bind = NULL;
	m_result_bind = NULL;
	//m_result = NULL;
	m_param_count = 0;
	m_result_count = 0;
}

pre_statement::~pre_statement()
{
	free();
}

int pre_statement::init(MYSQL *mysql, string& sql)
{
	m_mysql = mysql;
	m_stmt = mysql_stmt_init(mysql);
	if (!m_stmt)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:mysql_stmt_init( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, mysql_error(mysql));
		return 1;
	}
	if (mysql_stmt_prepare(m_stmt, sql.c_str(), sql.size()))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:mysql_stmt_prepare( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, mysql_stmt_error(m_stmt));
		return 1;
	}
	m_param_count = mysql_stmt_param_count(m_stmt);
	if (m_param_count > 0)
	{
		m_param_bind = new MYSQL_BIND[m_param_count];
		if (!m_param_bind)
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d:new MYSQL_BIND failed", __FILE__, __FUNCTION__, __LINE__);
			return 1;
		}
		memset(m_param_bind, 0, sizeof(MYSQL_BIND)*m_param_count);
	}
	return 0;
}

int pre_statement::set_param_bind(unsigned int index, uint8_t& value)
{
	if (index >= m_param_count)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:The index overflow", __FILE__, __FUNCTION__, __LINE__);
		return 1;
	}
	m_param_bind[index].buffer_type = MYSQL_TYPE_TINY;
	m_param_bind[index].buffer = &value;
	m_param_bind[index].buffer_length = sizeof(uint8_t);
	return 0;
}

int pre_statement::set_param_bind(unsigned int index, uint16_t& value)
{
	if (index >= m_param_count)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:The index overflow", __FILE__, __FUNCTION__, __LINE__);
		return 1;
	}
	m_param_bind[index].buffer_type = MYSQL_TYPE_SHORT;
	m_param_bind[index].buffer = &value;
	m_param_bind[index].buffer_length = sizeof(uint16_t);
	return 0;
}

int pre_statement::set_param_bind(unsigned int index, int32_t& value)
{
	if (index >= m_param_count)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:The index overflow", __FILE__, __FUNCTION__, __LINE__);
		return 1;
	}
	m_param_bind[index].buffer_type = MYSQL_TYPE_LONG;
	m_param_bind[index].buffer = &value;
	m_param_bind[index].buffer_length = sizeof(int32_t);
	return 0;
}

int pre_statement::set_param_bind(unsigned int index, uint32_t& value)
{
	if (index >= m_param_count)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:The index overflow", __FILE__, __FUNCTION__, __LINE__);
		return 1;
	}
	m_param_bind[index].buffer_type = MYSQL_TYPE_LONG;
	m_param_bind[index].buffer = &value;
	m_param_bind[index].buffer_length = sizeof(uint32_t);
	return 0;
}

int pre_statement::set_param_bind(unsigned int index, uint64_t& value)
{
	if (index >= m_param_count)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:The index overflow", __FILE__, __FUNCTION__, __LINE__);
		return 1;
	}
	m_param_bind[index].buffer_type = MYSQL_TYPE_LONGLONG;
	m_param_bind[index].buffer = &value;
	m_param_bind[index].buffer_length = sizeof(uint64_t);
	return 0;
}

int pre_statement::set_param_bind(unsigned int index, string& value)
{
	if (index >= m_param_count)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:The index overflow", __FILE__, __FUNCTION__, __LINE__);
		return 1;
	}
	m_param_bind[index].buffer_type = MYSQL_TYPE_VAR_STRING;
	m_param_bind[index].buffer = (char*)(value.c_str());
	//unsigned long length = value.length();
	m_param_bind[index].buffer_length = value.length();
	//m_param_bind[index].length = &length;
	//这个变量在mysql_stmt_execute执行的时候才会拿出来使用，如果传进来的length失效，会造成段错误，并且buffer_length赋值已经够用，为了稳固性弃用
	return 0;
}

int pre_statement::execute()
{
	if (!m_stmt)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:m_stmt has not been created", __FILE__, __FUNCTION__, __LINE__);
		return 1;
	}
	if (mysql_stmt_bind_param(m_stmt, m_param_bind))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:mysql_stmt_bind_param( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, mysql_stmt_error(m_stmt));
		return 1;
	}
	if (mysql_stmt_execute(m_stmt))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:mysql_stmt_execute( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, mysql_stmt_error(m_stmt));
		return 1;
	}
	return 0;
}

int pre_statement::query()
{
	if (!m_stmt)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:m_stmt Has not been created", __FILE__, __FUNCTION__, __LINE__);
		return 1;
	}
	if (mysql_stmt_bind_param(m_stmt, m_param_bind))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:mysql_stmt_bind_param( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, mysql_stmt_error(m_stmt));
		return 1;
	}
	if (mysql_stmt_execute(m_stmt))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:mysql_stmt_execute( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, mysql_stmt_error(m_stmt));
		return 1;
	}
	// 	m_result = mysql_stmt_result_metadata(m_stmt);
	// 	if (NULL == m_result)
	// 	{
	// 		log(LOG_ERROR, "[ERROR] %s:%s():%d:mysql_stmt_result_metadata( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, mysql_stmt_error(m_stmt));
	// 		return 1;
	// 	}
	// 	else
	// 	{
	// 		m_result_count = mysql_num_fields(m_result);
	// 		if (m_result_count > 0)
	// 		{
	// 			m_result_bind = new MYSQL_BIND[m_result_count];
	// 			if (!m_result_bind)
	// 			{
	// 				log(LOG_ERROR, "[ERROR] %s:%s():%d:new MYSQL_BIND failed", __FILE__, __FUNCTION__, __LINE__);
	// 				return 1;
	// 			}
	// 			memset(m_result_bind, 0, sizeof(MYSQL_BIND)*m_result_count);
	// 		}
	// 		else
	// 		{
	// 			log(LOG_ERROR, "[ERROR] %s:%s():%d:mysql_stmt_param_count( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, mysql_stmt_error(m_stmt));
	// 			return 1;
	// 		}
	// 	}
	m_result_count = mysql_stmt_field_count(m_stmt);
	if (m_result_count > 0)
	{
		m_result_bind = new MYSQL_BIND[m_result_count];
		if (!m_result_bind)
		{
			log(LOG_ERROR, "[ERROR] %s:%s():%d:new MYSQL_BIND failed", __FILE__, __FUNCTION__, __LINE__);
			return 1;
		}
		memset(m_result_bind, 0, sizeof(MYSQL_BIND)*m_result_count);
	}
	else
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:mysql_stmt_param_count( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, mysql_stmt_error(m_stmt));
		return 1;
	}
	return 0;
}

int pre_statement::set_param_result(unsigned int index, enum_field_types type, char *buffer, unsigned long buffer_length, unsigned long	*length)
{
	if (index >= m_result_count)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:The index overflow", __FILE__, __FUNCTION__, __LINE__);
		return 1;
	}
	m_result_bind[index].buffer_type = type;
	m_result_bind[index].buffer = buffer;
	m_result_bind[index].buffer_length = buffer_length;
	m_result_bind[index].length = length;
	//这个变量在mysql_stmt_execute执行的时候才会拿出来使用，用来告诉得到的结果长度
	return 0;
}

int pre_statement::get_result()
{
	if (mysql_stmt_bind_result(m_stmt, m_result_bind))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:mysql_stmt_bind_result( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, mysql_stmt_error(m_stmt));
		return 1;
	}
	//其实可以直接mysql_stmt_fetch，但是因为要用到mysql_stmt_num_rows，只好mysql_stmt_store_result
	if (mysql_stmt_store_result(m_stmt))
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:mysql_stmt_store_result( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, mysql_stmt_error(m_stmt));
		return 1;
	}
	return 0;
}

unsigned long long pre_statement::get_num_rows()
{
	return mysql_stmt_num_rows(m_stmt);
}

int pre_statement::fetch_result()
{
	int ret = mysql_stmt_fetch(m_stmt);
	if (ret == 0)
	{
		return 0;
	}
	else if (ret == MYSQL_NO_DATA)
	{
		return 1;
	}
	else if (ret == MYSQL_DATA_TRUNCATED)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:MYSQL_DATA_TRUNCATED", __FILE__, __FUNCTION__, __LINE__);
		return 1;
	}
	else if (ret == 1)
	{
		log(LOG_ERROR, "[ERROR] %s:%s():%d:%s", __FILE__, __FUNCTION__, __LINE__, mysql_stmt_error(m_stmt));
		return 1;
	}
	return 1;
}

void pre_statement::free()
{
	if (m_stmt)
	{
		mysql_stmt_free_result(m_stmt);
		mysql_stmt_close(m_stmt);
		m_stmt = NULL;
		do
		{
			MYSQL_RES *result = mysql_store_result(m_mysql);
			if (NULL != result)
			{
				mysql_free_result(result);
				result = NULL;
			}
		} while (0 == mysql_next_result(m_mysql));
	}
	if (m_param_bind)
	{
		delete[]m_param_bind;
		m_param_bind = NULL;
	}
	if (m_result_bind)
	{
		delete[]m_result_bind;
		m_result_bind = NULL;
	}
	// 	if (NULL != m_result)
	// 	{
	// 		mysql_free_result(m_result);
	// 		m_result = NULL;
	// 	}
	m_param_count = 0;
	m_result_count = 0;
}
