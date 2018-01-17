#ifndef _PRE_STATEMENT_H_
#define _PRE_STATEMENT_H_
/*****************************************************************************************
*Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
*文件名称：pre_statement.h
*描		述：MySQL预处理语句操作类
*当前版本：1.0
*作		者：zhangcp
*创建日期：2016-06-27 14:00
*修改日期：2016-06-27
*******************************************************************************************/
#include "mysql.h"
#include <stdint.h>
#include <string>
using namespace std;

class pre_statement
{
public:
	pre_statement();
	~pre_statement();

	int init(MYSQL *mysql, string& sql);

	int set_param_bind(unsigned int index, uint8_t& value);
	int set_param_bind(unsigned int index, uint16_t& value);
	int set_param_bind(unsigned int index, int32_t& value);
	int set_param_bind(unsigned int index, uint32_t& value);
	int set_param_bind(unsigned int index, uint64_t& value);
	int set_param_bind(unsigned int index, string& value);

	int execute();

	int query();
	int set_param_result(unsigned int index, enum_field_types type, char *buffer, unsigned long buffer_length, unsigned long	*length);
	int get_result();
	unsigned long long get_num_rows();
	int fetch_result();

	void free();

private:
	MYSQL *m_mysql;
	MYSQL_STMT *m_stmt;
	MYSQL_BIND *m_param_bind;
	MYSQL_BIND *m_result_bind;
	//MYSQL_RES *m_result;
	unsigned long m_param_count;
	unsigned int m_result_count;
};

#endif
