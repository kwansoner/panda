/************************************************
* 				mutex
* 
* desc: pthread mutex封装
* author: kwanson
* email: CSDN kwanson
*************************************************/

#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <pthread.h>

#include "utils.h"
#include "cond.h"

namespace Pthread
{
class IMutex
{
	public:
		virtual ~IMutex(){};

		// desc: 加锁
		// param: void
		// return: 0/成功返回 -1/错误
		virtual int lock() = 0;

		// desc: 尝试加锁
		// param: void
		// return: 0/成功返回 -1/错误
		virtual int trylock() = 0;

		// desc: 解锁
		// param: void
		// return: 0/成功返回 -1/错误
		virtual int unlock() = 0;
};
class CMutex: public IMutex, private IUncopyable
{
	friend class CCond;
	public:
		CMutex();
	    ~CMutex();
		inline int lock();
		inline int trylock();
		inline int unlock();
	private:
		pthread_mutex_t m_mutex;
};
class CGuard
{
	public:
		CGuard(IMutex &mutex): m_mutex(mutex){m_mutex.lock();};
		~CGuard(){m_mutex.unlock();};
	private:
		IMutex &m_mutex;
};
}
#endif