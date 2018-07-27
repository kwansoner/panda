/************************************************
* 				cond
* 
* desc: pthread 条件变量封装声明
* author: kwanson
* email: CSDN kwanson
*************************************************/

#ifndef __COND_H__
#define __COND_H__

#include <pthread.h>

namespace Pthread
{
class CMutex;
class ICond
{
	public:
		virtual ~ICond(){};

		// desc: 等待条件变量
		// param: mutex/互斥锁
		// return: 0/成功返回 -1/错误
		virtual int wait(CMutex &) = 0;

		// desc: 释放条件变量
		// param: void 
		// return: 0/成功返回 -1/错误
		virtual int signal() = 0;

		// desc: 广播条件变量
		// param: void 
		// return: 0/成功返回 -1/错误
		virtual int broadcast() = 0;
};
class CCond: public ICond
{
	public:
		CCond();
		~CCond();
		int wait(CMutex &);
		int signal();
		int broadcast();
	private:
		pthread_cond_t m_cond;
};
}
#endif
