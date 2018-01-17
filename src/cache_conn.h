#ifndef _CACHE_CONN_H_
#define _CACHE_CONN_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：cache_conn.h
 *描		述：Redis操作类
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2016-09-23 14:00 
 *修改日期：2016-09-23
*******************************************************************************************/
#include "../hiredis/hiredis.h"
#include <string>
using namespace std;

class cache_conn
{
public:
	cache_conn();
	~cache_conn();

	int init();
	string set(string key, string value);
	string get(string key);
	bool del(string key);

private:
	redisContext *m_context;
};

#endif
