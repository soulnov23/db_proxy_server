#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：thread_pool.h
 *描		述：线程池操作类
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2016-06-27 14:00
 *修改日期：2016-06-27
*******************************************************************************************/
#include "task.h"
#include "base_thread.h"
#include "mysql_conn.h"
#include "cache_conn.h"
#include "thread_lock.h"
#include <deque>
using namespace std;

//对初始线程进行继承扩展，该线程维持了一个task任务链表，对task链表进行轮询，执行task的run方法
class task_thread : public base_thread
{
public:
	task_thread();
	~task_thread();

	int init();
	void run();
	void push_task(task *ptask);

private:
	mysql_conn m_mysql_conn;
	cache_conn *m_cache_conn;
	thread_lock m_thread_lock;
	deque<task *> m_deque_task;
	bool m_flag;
};

//线程池中的线程都是task线程，用随机数的方法对线程池中的某个task线程增加task任务到他的task链表中去
class thread_pool
{
public:
	thread_pool();
	~thread_pool();

	int init();
	void destory();
	void add_task(task *ptask);

private:
	int m_pool_size;
	task_thread *m_task_thread_array;
	int m_task_index;
};

#endif