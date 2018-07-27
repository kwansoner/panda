/************************************************
* 				event
* 
* desc: epoll事件中心实现
* author: kwanson
* email: CSDN kwanson
*************************************************/

#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>

#include "event.h"
#include "log.h"
#include "utils.h"

namespace Event
{
int IEventHandle::register_event(int fd, EventType type)
{
	return CEventProxy::instance()->register_event(fd, this, type);
}
int IEventHandle::register_event(Socket::ISocket &socket, EventType type)
{
	return CEventProxy::instance()->register_event(socket, this, type);
}
int IEventHandle::shutdown_event(int fd)
{
	return CEventProxy::instance()->shutdown_event(fd);
}
int IEventHandle::shutdown_event(Socket::ISocket &socket)
{
	return CEventProxy::instance()->shutdown_event(socket);
}
CNetObserver::CNetObserver(INetObserver &obj, EventType regevent): 
	m_regevent(regevent), m_obj(obj), m_refcount(1)
{
}
CNetObserver::~CNetObserver()
{
}
void CNetObserver::addref()
{
	Pthread::CGuard guard(m_refcount_mutex);
	m_refcount++;
}
inline void CNetObserver::subref()
{
	Pthread::CGuard guard(m_refcount_mutex);
	m_refcount--;
}
bool CNetObserver::subref_and_test()
{
	Pthread::CGuard guard(m_refcount_mutex);
	m_refcount--;
	return (m_refcount == 0x00);
}
void CNetObserver::selfrelease()
{
	delete this;
}
EventType CNetObserver::get_regevent()
{
	return m_regevent;
}
const INetObserver *CNetObserver::get_handle()
{
	return &m_obj;
}
void CNetObserver::handle_in(int fd)
{
	m_obj.handle_in(fd);
}
void CNetObserver::handle_out(int fd)
{
	m_obj.handle_out(fd);
}
void CNetObserver::handle_close(int fd)
{
	m_obj.handle_close(fd);
}
void CNetObserver::handle_error(int fd)
{
	m_obj.handle_error(fd);
}
CEvent::CEvent(size_t neventmax):   m_detectionthread(0)
{
	bzero((void *)&m_eventbuff, sizeof(m_eventbuff));
	
	m_epollfd = epoll_create(neventmax);
	assert(m_epollfd >= 0);

	m_ithreadpool = ThreadPool::CThreadPoolProxy::instance();
	
	pthread_t tid = 0;
	if(pthread_create(&tid, NULL, eventwait_thread, (void *)this) == 0)
		m_detectionthread = tid;
}
CEvent::~CEvent()
{
	if(m_detectionthread != 0 && pthread_cancel(m_detectionthread) == 0){	//cancel thread
		pthread_join(m_detectionthread, (void **)NULL);
	}
	
	if(!INVALID_FD(m_epollfd))
		close(m_epollfd);
}
int CEvent::register_event(int fd, IEventHandle *handle, EventType type)
{
	if(INVALID_FD(fd) || INVALID_FD(m_epollfd) || INVALID_POINTER(handle)){
		seterrno(EINVAL);
		return -1;
	}

	struct epoll_event newevent;
	newevent.data.fd = fd;
	newevent.events = type;
	
	ExistRet ret = isexist(fd, type, handle);
	if(ret == Existed)
		return 0;

	/*
	* epoll_ctl先执行会出现注册后立即产生事件
	* 但是此时未执行到record记录导致丢失事件的问题
	*/
	record(fd, type, handle);
	if(ret == HandleModify)
		return 0;

	int opt;
	if(ret == TypeModify || ret == Modify)
		opt = EPOLL_CTL_MOD;
	else if(ret == NotExist)
		opt = EPOLL_CTL_ADD;

	if(epoll_ctl(m_epollfd, opt, fd, &newevent) < 0){
			errsys("epoll op %d, fd %#x error\n", opt, fd);
			detach(fd, true);
			return -1;	
	}
	
	return 0;
}
int CEvent::unregister_event(int fd)
{
	if(epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, NULL) < 0){
		errsys("epoll delete fd %d failed\n", fd);
		return -1;
	}
	
	return detach(fd);
}
int CEvent::shutdown_event(int fd)
{
	trace("sock[%#X] shutdown event\n", fd);
	return ::shutdown(fd, SHUT_WR);
}
CEvent::ExistRet CEvent::isexist(int fd, EventType type, IEventHandle *handle)
{
	Pthread::CGuard guard(m_eventreg_mutex);
	EventMap_t::iterator itor = m_eventreg.find(fd);
	if(itor == m_eventreg.end()){	// not existed
		return NotExist;
	}

	CNetObserver &eventhandle = *(itor->second);
	EventType oldregtype = eventhandle.get_regevent();
	const INetObserver *oldreghandle = eventhandle.get_handle();
	if(oldregtype != type && oldreghandle != handle) //Whole modify
		return Modify;
	else if(oldregtype != type)
		return TypeModify;
	else if(oldreghandle != handle)
		return HandleModify;

	return Existed;		
}
int CEvent::record(int fd, EventType type, IEventHandle *handle)
{
	CNetObserver *newobserver = new CNetObserver(*handle, type);
	assert(newobserver != NULL);
	
	Pthread::CGuard guard(m_eventreg_mutex);
	m_eventreg[fd] = newobserver;
	return 0;
}
int CEvent::detach(int fd, bool release)
{
	Pthread::CGuard guard(m_eventreg_mutex);
	EventMap_t::iterator itor = m_eventreg.find(fd);
	if(itor == m_eventreg.end()){	// not existed
		return -1;
	}

	if(release)
		itor->second->selfrelease();
		
	m_eventreg.erase(itor);
	return 0;
}
CNetObserver *CEvent::get_observer(int fd)
{
	Pthread::CGuard guard(m_eventreg_mutex);
	EventMap_t::iterator itor = m_eventreg.find(fd);
	if(itor == m_eventreg.end())
		return NULL;
		
	CNetObserver *observer = m_eventreg[fd];
	observer->addref();
	return observer;
}
int CEvent::pushtask(int fd, EventType event)
{
	Pthread::CGuard guard(m_events_mutex);
	EventTask_t::iterator itor = m_events.find(fd);
	if(itor == m_events.end()){
		m_events[fd] = event;
		return 0;
	}
							// exist, update it
	itor->second = (EventType)(itor->second | event);	
	return 0;
}
int CEvent::poptask(int &fd, EventType &event)
{
	Pthread::CGuard guard(m_events_mutex);
	EventTask_t::iterator itor = m_events.begin();
	if(itor == m_events.end())
		return -1;

	fd = itor->first;
	event = itor->second;

	m_events.erase(itor);
	return 0;
}
int CEvent::cleartask(int fd)
{
	Pthread::CGuard guard(m_events_mutex);
	if(fd == -1){	// clear all
		m_events.clear();
		return 0;
		
	}else if(fd >= 0){
		EventTask_t::iterator itor = m_events.find(fd);
		if(itor == m_events.end())
			return -1;

		m_events.erase(itor);
		return 0;	
	}

	return -1;
}
size_t CEvent::tasksize()
{
	Pthread::CGuard guard(m_events_mutex);
	return m_events.size();
}
void CEvent::threadhandle()
{
	int fd = 0x00;
	EventType events;
	if(poptask(fd, events) < 0){
		return;
	}
	CNetObserver *observer = get_observer(fd);
	if(observer == NULL)
		return;

	/*
	* 关闭时递减引用计数。在对象的所有回调处理完时真正释放
	*/
	if(events & ECLOSE){
		cleartask(fd);		
		observer->subref();	
		
	}else{	
		if(events & EERR){
			observer->handle_error(fd);
		}
		if(events & EIN){
			observer->handle_in(fd);
		}
		if(events & EOUT){
			observer->handle_out(fd);
		}
		
	}	
	
	/*
	* unregister_event 执行后于handle_close将会出现当前套接字关闭后
	* 在仍未执行完unregister_event时新的连接过来，得到一样的描述符
	* 新的连接调用register_event却未注册进入epoll。同时han_close中
	* 关闭了套接字，unregister_event中epoll删除关闭的套接字报错
	*/
	if(observer->subref_and_test()){
		unregister_event(fd);
		observer->handle_close(fd);	
		observer->selfrelease();
	}	
}
void *CEvent::eventwait_thread(void *arg)
{
	CEvent &cevent = *(CEvent *)(arg);
	if(INVALID_FD(cevent.m_epollfd)){
		seterrno(EINVAL);
		pthread_exit(NULL);
	}

	for(;;){
		int nevent = epoll_wait(cevent.m_epollfd, &cevent.m_eventbuff[0], EventBuffLen, -1);
		if(nevent < 0 && errno != EINTR){
			errsys("epoll wait error\n");
			break;
		}

		for(int i = 0; i < nevent; i++){

			int fd = cevent.m_eventbuff[i].data.fd;
			EventType events = static_cast<EventType>(cevent.m_eventbuff[i].events);
			
			if(cevent.pushtask(fd, events) == 0x00){
				cevent.m_ithreadpool->pushtask(&cevent);
			}
		}
	}

	pthread_exit(NULL);
}
CEventProxy::CEventProxy(size_t neventmax)
{
	m_event = new CEvent(neventmax);
}
CEventProxy::~CEventProxy()
{
	if(m_event)
		delete m_event;
}
CEventProxy *CEventProxy::instance()
{
	static CEventProxy *eventproxy = NULL;
	if(eventproxy == NULL)
		eventproxy = new CEventProxy(NEVENT_MAX); 
	return eventproxy;
}
int CEventProxy::register_event(int fd, IEventHandle * handle, EventType type)
{
	return m_event->register_event(fd, handle, type);
}
int CEventProxy::register_event(Socket::ISocket &socket, IEventHandle * handle, EventType type)
{
	return register_event(socket.fd(), handle, type);
}
int CEventProxy::shutdown_event(int fd)
{
	return m_event->shutdown_event(fd);
}
int CEventProxy::shutdown_event(Socket::ISocket &socket)
{
	return m_event->shutdown_event(socket.fd());
}
}