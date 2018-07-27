/************************************************
* 				cond
* 
* desc: pthread 条件变量封装
* author: kwanson
* email: CSDN kwanson
*************************************************/

#include "cond.h"
#include "mutex.h"

namespace Pthread
{
CCond::CCond()
{
	pthread_cond_init(&m_cond, NULL);
}
CCond::~CCond()
{
	pthread_cond_destroy(&m_cond);
}
int CCond::wait(CMutex &mutex)
{
	return pthread_cond_wait(&m_cond, &mutex.m_mutex);
}
int CCond::signal()
{
	return pthread_cond_signal(&m_cond);
}
int CCond::broadcast()
{
	return pthread_cond_broadcast(&m_cond);
}
}
