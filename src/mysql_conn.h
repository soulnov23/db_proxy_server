#ifndef _MYSQL_CONN_H_
#define _MYSQL_CONN_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：mysql_conn.h
 *描		述：MySQL连接对象
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2016-12-30 14:00
 *修改日期：2016-12-30
*******************************************************************************************/
#include "mysql.h"

class mysql_conn
{
public:
	mysql_conn();
	~mysql_conn();

public:
	int init();
	void free();
	MYSQL *get_mysql();
	
private:
	MYSQL *m_mysql;
};

#endif
