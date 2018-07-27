/************************************************
* 				threadpool
* 
* desc: 线程池
* author: kwanson
* email: CSDN kwanson
*************************************************/

#include "threadpool.h"
#include "log.h"
#include "utils.h"

namespace ThreadPool
{
CThreadPool::CThreadPool(size_t threadnum, size_t tasknum): 
	m_taskqueue(tasknum), m_hasleader(false)
{
	m_threadnum = (threadnum > THREADNUM_MAX)? THREADNUM_MAX: threadnum;
	create_threadpool();
}
CThreadPool::~CThreadPool() 
{
	destroy_threadpool();
}
int CThreadPool::pushtask(IThreadHandle *handle, bool block)
{
	if(block)
		return m_taskqueue.push(handle);
	return m_taskqueue.push_nonblock(handle);
}
void CThreadPool::promote_leader()
{
	Pthread::CGuard guard(m_identify_mutex);
	while(m_hasleader){	// more than one thread can return
		m_befollower_cond.wait(m_identify_mutex);
	}
	m_hasleader = true;
}
void CThreadPool::join_follwer()
{
	Pthread::CGuard guard(m_identify_mutex);
	m_hasleader = false;
	m_befollower_cond.signal();
}
void CThreadPool::create_threadpool()
{
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	for(size_t i = 0; i < m_threadnum; i++){
		pthread_t tid = 0;
		if(pthread_create(&tid, &thread_attr, process_task, (void *)this) < 0){
			errsys("create thread[%d] filed\n", (int)i);
			continue;
		}

		m_thread.push_back(tid);
	}
	pthread_attr_destroy(&thread_attr);
	trace("create thread pool, thread number %d\n", (int)m_thread.size());
}
void CThreadPool::destroy_threadpool()
{
	void *retval = NULL;
	vector_tid_t::iterator itor = m_thread.begin();
	for(; itor != m_thread.end(); itor++){
		if(pthread_cancel(*itor) < 0 || pthread_join(*itor, &retval) < 0){
			errsys("destroy thread[%d]\n", (int)(*itor));
			continue;
		}
	}

	m_thread.clear();
	trace("destroy thread pool... done\n");
}
void *CThreadPool::process_task(void * arg)
{
	CThreadPool &threadpool = *(CThreadPool *)arg;
	while(true){
		threadpool.promote_leader();	
		IThreadHandle *threadhandle = NULL;
		int ret = threadpool.m_taskqueue.pop(threadhandle);
		threadpool.join_follwer();		

		if(ret == 0 && threadhandle)
			threadhandle->threadhandle();
	}	

	pthread_exit(NULL);
}

}
