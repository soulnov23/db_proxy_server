#include "config_file_oper.h"
#include <string.h>
#include "im_log.h"

config_file_oper::config_file_oper(const char *file_name)
{
	map<string, string>().swap(m_map_config);
	m_load = false;
	load_file(file_name);
}

config_file_oper::~config_file_oper()
{
	map<string, string>().swap(m_map_config);
}

void config_file_oper::load_file(const char *file_name)
{
	FILE *fp = fopen(file_name, "r");
	if (!fp)
	{		
		log(LOG_ERROR, "[ERROR] %s:%s():%d:fopen( ) failed:%s", __FILE__, __FUNCTION__, __LINE__, strerror(errno));
		return;
	}
	char buf[256];
	while (true)
	{
		char *p = fgets(buf, 256, fp);
		if (!p)
		{
			break;
		}
		int len = strlen(buf);
		if ((buf[len-1] == '\n') || (buf[len-1] == '\r'))
		{
			buf[len - 1] = 0;
		}
		if (buf[len-2] == '\r')
		{
			buf[len - 2] = 0;
		}
		char *ch = strchr(buf, '#');
		if (ch)
		{
			*ch = 0;
		}
		if (strlen(buf) == 0)
		{
			continue;
		}
		parse_line(buf);
	}
	fclose(fp);
	m_load = true;
}

void config_file_oper::parse_line(char *line)
{
	char *p = strchr(line, '=');
	if (NULL == p)
	{
		return;
	}
	*p = 0;
	char *key = trim_space(line);
	char *value = trim_space(p + 1);
	if (key && value)
	{
		m_map_config.insert(make_pair(key, value));
	}
}

char *config_file_oper::trim_space(char *name)
{
	char *start_pos = name;
	while ((*start_pos == ' ') || (*start_pos == '\t'))
	{
		start_pos++;
	}
	if (strlen(start_pos) == 0)
	{
		return NULL;
	}
	char *end_pos = name + strlen(name) - 1;
	while ((*end_pos == ' ') || (*end_pos == '\t'))
	{
		*end_pos = 0;
		end_pos--;
	}
	int len = (int)(end_pos - start_pos) + 1;
	if (len <= 0)
	{
		return NULL;
	}
	return start_pos;
}

char *config_file_oper::get_config_value(const char *name)
{
	if (!m_load)
	{
		return NULL;
	}
	char *value = NULL;
	map<string, string>::iterator it = m_map_config.find(name);
	if (it != m_map_config.end()) 
	{
		value = (char*)it->second.c_str();
	}
	return value;
}