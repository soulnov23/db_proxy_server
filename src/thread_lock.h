#ifndef _THREAD_LOCK_H_
#define _THREAD_LOCK_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：pthread_lock.h
 *描		述：线程锁
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2016-06-27 14:00
 *修改日期：2016-11-24 14:00(增加windows的条件变量)
*******************************************************************************************/
#ifdef WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif // WIN32

class thread_lock
{
public:
	thread_lock()
	{
#ifdef WIN32
		InitializeCriticalSection(&m_section);
		InitializeConditionVariable(&m_conditon);
#else
		pthread_mutexattr_init(&m_mutexatr);
		//互斥锁重复上锁，不会引起死锁，一个线程对这类互斥锁的多次重复上锁必须由这个线程来重复相同数量的解锁，这样才能解开这个互斥锁，别的线程才能得到这个互斥锁
		//pthread_mutexattr_settype(&m_mutexatr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&m_mutex, &m_mutexatr);
		pthread_cond_init(&m_cond, NULL);
#endif // WIN32
	}

	~thread_lock()
	{
#ifdef WIN32
		DeleteCriticalSection(&m_section);
#else
		pthread_mutexattr_destroy(&m_mutexatr);
		pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy(&m_cond);
#endif // WIN32
	}
	
	void lock()
	{
#ifdef WIN32
		EnterCriticalSection(&m_section);
#else
		pthread_mutex_lock(&m_mutex);
#endif // WIN32
	}

	void unlock()
	{
#ifdef WIN32
		LeaveCriticalSection(&m_section);
#else
		pthread_mutex_unlock(&m_mutex);
#endif // WIN32
	}

	void wait()
	{
#ifdef WIN32
		SleepConditionVariableCS(&m_conditon, &m_section, INFINITE);
#else
		pthread_cond_wait(&m_cond, &m_mutex);
#endif // WIN32
	}

	void signal()
	{
#ifdef WIN32
		WakeConditionVariable(&m_conditon);
#else
		pthread_cond_signal(&m_cond);
#endif // WIN32
	}

private:
#ifdef WIN32
	CRITICAL_SECTION m_section;
	CONDITION_VARIABLE m_conditon;
#else
	pthread_mutex_t m_mutex;
	pthread_mutexattr_t m_mutexatr;
	pthread_cond_t m_cond;
#endif // WIN32
};

#endif
/*
//                            _ooOoo_  
//                           o8888888o  
//                           88" . "88  
//                           (| -_- |)  
//                            O\ = /O  
//                        ____/`---'\____  
//                      .   ' \\| |// `.  
//                       / \\||| : |||// \  
//                     / _||||| -:- |||||- \  
//                       | | \\\ - /// | |  
//                     | \_| ''\---/'' | |  
//                      \ .-\__ `-` ___/-. /  
//                   ___`. .' /--.--\ `. . __  
//                ."" '< `.___\_<|>_/___.' >'"".  
//               | | : `- \`.;`\ _ /`;.`/ - ` : | |  
//                 \ \ `-. \_ __\ /__ _/ .-` / /  
//         ======`-.____`-.___\_____/___.-`____.-'======  
//                            `=---='  
//  
//         .............................................  
//                  佛祖保佑             永无BUG 
//          佛曰:  
//                  写字楼里写字间，写字间里程序员；  
//                  程序人员写程序，又拿程序换酒钱。  
//                  酒醒只在网上坐，酒醉还来网下眠；  
//                  酒醉酒醒日复日，网上网下年复年。  
//                  但愿老死电脑间，不愿鞠躬老板前；  
//                  奔驰宝马贵者趣，公交自行程序员。  
//                  别人笑我忒疯癫，我笑自己命太贱；  
//                  不见满街漂亮妹，哪个归得程序员？ 
*/
