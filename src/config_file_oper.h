#ifndef _CONFIG_FILE_OPER_H_
#define _CONFIG_FILE_OPER_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：config_file_oper.h
 *描		述：conf文件读写类
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2016-06-27 14:00
 *修改日期：2016-06-27
*******************************************************************************************/
#include <map>
using namespace std;

class config_file_oper
{
public:
	config_file_oper(const char *file_name);
	~config_file_oper();
	char *get_config_value(const char *name);

private:
	void load_file(const char *file_name);
	void parse_line(char *line);
	char *trim_space(char *name);

private:
	map<string, string> m_map_config;
	bool m_load;
};

#endif