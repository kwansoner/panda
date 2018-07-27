/************************************************
* 				server
* 
* desc: socket server封装
* author: kwanson
* email: CSDN kwanson
*************************************************/

#include <unistd.h>
#include <assert.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "utils.h"
#include "sockopt.h"
#include "client.h"
#include "server.h"

namespace Socket
{
CStreamServer::CStreamServer(const std::string &addr, uint16_t port):
m_addr(addr), m_port(port), m_sockfd(-1)
{
}
CStreamServer::~CStreamServer()
{
	close();	// ignore error
}
int CStreamServer::fd()
{
	Pthread::CGuard guard(m_socketmutex);
	return m_sockfd;
}
int CStreamServer::set_nonblock(bool nonblock)
{
	Pthread::CGuard guard(m_socketmutex);
	if(INVALID_FD(m_sockfd))	
		return -1;
		
	return Socket::set_nonblock(m_sockfd, nonblock);
}
int CStreamServer::start(size_t backlog)
{
	Pthread::CGuard guard(m_socketmutex);
	if(!INVALID_FD(m_sockfd))	
		return 0;
		
	struct sockaddr_in sockaddr;
	if(convert_inaddr(m_addr, m_port, sockaddr) < 0)
		return -1;

	int sockfd = 0x00;
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;

	set_resuseport(sockfd, true);	// ignore error
	if(bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0){
		::close(sockfd);
		return -1;
	}
	
	if(listen(sockfd, backlog) < 0){
		::close(sockfd);
		return -1;
	}
	
	m_sockfd = sockfd;
	return 0;	
}
int CStreamServer::close()
{
	Pthread::CGuard guard(m_socketmutex);
	if(INVALID_FD(m_sockfd))
		return -1;

	if(::close(m_sockfd) < 0);
		return -1;
		
	m_sockfd = -1;
	return 0;
}
bool CStreamServer::isclose()
{
	Pthread::CGuard guard(m_socketmutex);
	if(INVALID_FD(m_sockfd))
		return true;
	return false;
}
IClient *CStreamServer::accept()
{
	Pthread::CGuard guard(m_socketmutex);
	if(INVALID_FD(m_sockfd))
		return NULL;

	struct sockaddr_in clientaddr;
	socklen_t clientaddrlen = sizeof(clientaddr);
	int clientfd = ::accept(m_sockfd, (struct sockaddr *)&clientaddr, &clientaddrlen);
	if(INVALID_FD(clientfd))
		return NULL;

	IClient *client = new CStreamClient(clientfd);
	assert(client != NULL);
	return client;
}

}
