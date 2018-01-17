#include "im_log.h"
#include <stdarg.h>
im_log::im_log(const std::string& initFileName)
{
	try
	{
		log4cpp::PropertyConfigurator::configure(initFileName);//导入配置文件
		log4cpp::Category& log = log4cpp::Category::getInstance(std::string("error"));
		m_log = &log;
		if (!m_log)
		{
			std::cout << "Configure error " << std::endl;
		}
	}
	catch (log4cpp::ConfigureFailure& f)
	{
		std::cout << "Configure Problem " << f.what() << std::endl;
	}
}
im_log::~im_log()
{
	log4cpp::Category::shutdown();
}

void im_log::debug(const std::string& log)
{
	if (m_log)
	{
		return m_log->debug(log);
	}
}
void im_log::warn(const std::string& log)
{
	if (m_log)
	{
		return m_log->warn(log);
	}
}
void im_log::error(const std::string& log)
{
	if (m_log)
	{
		return m_log->error(log);
	}
}
void im_log::fatal(const std::string& log)
{
	if (m_log)
	{
		return m_log->fatal(log);
	}
}
void im_log::crit(const std::string& log)
{
	if (m_log)
	{
		return m_log->crit(log);
	}
}
void im_log::emerg(const std::string& log)
{
	if (m_log)
	{
		return m_log->emerg(log);
	}
}
void  log(LOGLEVEL level, const char *format, ...)
{
	static im_log g_log(IM_COMFIG);
	char buf[2048];
	memset(buf, 0, 2048);
	va_list vl;
	va_start(vl, format); 
	vsnprintf(buf,2048,format,vl);
	va_end(vl);
	string log(buf);
	switch (level)
	{
	case LOG_FATAL:
		g_log.fatal(log);
		break;
	case LOG_CRIT:
		g_log.crit(log);
		break;
	case LOG_ERROR:
		g_log.error(log);
		break;
	case LOG_WARN:
		g_log.warn(log);
		break;
	case LOG_DEBUG:
		g_log.debug(log);
		break;
	default:
		g_log.debug(log);
		break;
	}
}
