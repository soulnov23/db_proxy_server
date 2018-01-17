#include "thread_pool.h"
#include "config_file_oper.h"
#include "im_log.h"

task_thread::task_thread()
{
	deque<task *>().swap(m_deque_task);
	m_flag = true;
}

task_thread::~task_thread()
{
	m_flag = false;
	m_thread_lock.lock();
	m_thread_lock.signal();
	m_thread_lock.unlock();
	wait();
	m_mysql_conn.free();
	if (NULL != m_cache_conn)
	{
		delete m_cache_conn;
		m_cache_conn = NULL;
	}
}

int task_thread::init()
{
	int ret = 1;
	if (create())
	{
		if (0 == m_mysql_conn.init())
		{
			m_cache_conn = new cache_conn;
			if (NULL != m_cache_conn)
			{
				ret = 0;
			}
		}
	}
	return ret;
}

void task_thread::run()
{
	while (m_flag)
	{
		m_thread_lock.lock();
		if (!m_deque_task.empty())
		{
			auto ptask = m_deque_task.front();
			m_deque_task.pop_front();
			m_thread_lock.unlock();
			ptask->run(m_mysql_conn.get_mysql(), m_cache_conn);
		}
		else
		{
			m_thread_lock.wait();
			m_thread_lock.unlock();
		}

	}
}

void task_thread::push_task(task *ptask)
{
	m_thread_lock.lock();
	m_deque_task.push_back(ptask);
	m_thread_lock.signal();
	m_thread_lock.unlock();
}

thread_pool::thread_pool()
{
	m_task_index = 0;
}

thread_pool::~thread_pool()
{

}

int thread_pool::init()
{
	int ret = 1;
	config_file_oper config_file("server_config.conf");
	m_pool_size = atoi(config_file.get_config_value("thread_pool_size"));
	m_task_thread_array = new task_thread[m_pool_size];
	if (!m_task_thread_array)
	{
		return 1;
	}
	int flag = 0;
	for (int i = 0; i < m_pool_size; i++)
	{
		if (0 == m_task_thread_array[i].init())
		{
			flag++;
		}
	}
	if (flag == m_pool_size)
	{
		log(LOG_DEBUG, "Thread pool started successfully");
		ret = 0;
	}
	else
	{
		log(LOG_ERROR, "[ERROR] Thread pool started failed and num = %d", flag);
	}
	return ret;
}

void thread_pool::destory()
{
	if (m_task_thread_array)
	{
		delete [] m_task_thread_array;
		m_task_thread_array = NULL;
	}
}

void thread_pool::add_task(task *ptask)
{
	if (m_task_index == m_pool_size)
	{
		m_task_index = 0;
	}
	m_task_thread_array[m_task_index].push_task(ptask);
	m_task_index++;
}