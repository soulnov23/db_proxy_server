#ifndef _GET_ATOMIC_H_
#define _GET_ATOMIC_H_
/*****************************************************************************************
 *Copyright (C) 2007-2016 DriveTheLife. All Rights Reserved.
 *文件名称：get_atomic.h
 *描		述：
 *当前版本：1.0
 *作		者：zhangcp
 *创建日期：2017-04-14 14:00
 *修改日期：2017-04-14 14:00
*******************************************************************************************/
#include <time.h>
#include <stdint.h>
#include <atomic>

std::atomic<uint32_t> atomic_index(0);

uint64_t get_atomic()
{
	uint64_t seconds = time(NULL);
	seconds = 32 << seconds;
	seconds |= atomic_index++;
	return seconds;
}

#endif
