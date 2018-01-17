#pragma  once
#include<iostream>
#include <string.h>
#include<log4cpp/Category.hh>
#include<log4cpp/OstreamAppender.hh>
#include<log4cpp/Priority.hh>
#include<log4cpp/PatternLayout.hh>
#include<log4cpp/FileAppender.hh>
#include<log4cpp/Category.hh>
#include<log4cpp/PropertyConfigurator.hh>
using namespace std;
#define IM_COMFIG string("log_config.conf")
/**************************************************************
日志类型
***************************************************************/
typedef enum
{	
	LOG_DEBUG =0,
	LOG_WARN ,
	LOG_ERROR,
	LOG_CRIT,
	LOG_FATAL,
}LOGLEVEL;
/**************************************************************
类名 ：im_log
作用 ：日志类
***************************************************************/
class im_log
{
public:
	im_log(const std::string& initFileName);
	~im_log();
	void debug(const std::string& log);
	void warn(const std::string& log);
	void error(const std::string& log);
	void fatal(const std::string& log);
	void crit(const std::string& log);
	void emerg(const std::string& log);
private:
	log4cpp::Category *m_log;
};
extern void log(LOGLEVEL level, const char *format, ...);//最大长度2k 超出不打印
