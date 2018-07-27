/************************************************
* 				client
* 
* desc: socket client定义
* author: kwanson
* email: CSDN kwanson
*************************************************/

#include <errno.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <arpa/inet.h>

#include "utils.h"
#include "sockopt.h"
#include "client.h"

namespace Socket
{

CStreamClient::CStreamClient(): m_sockfd(-1)
{
}
CStreamClient::CStreamClient(int fd): m_sockfd(fd)
{
}
CStreamClient::CStreamClient(const CStreamClient &streamclient)
{
	m_sockfd = streamclient.m_sockfd;
}
CStreamClient::~CStreamClient()
{
	close();	// ignore error
}
int CStreamClient::fd()
{
	Pthread::CGuard guard(m_socketmutex);
	return m_sockfd;
}
int CStreamClient::start()
{
	Pthread::CGuard guard(m_socketmutex);
	if(!INVALID_FD(m_sockfd))
		return 0;
	if( (m_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return -1;
	return 0;
}
int CStreamClient::close()
{
	Pthread::CGuard guard(m_socketmutex);
	if(INVALID_FD(m_sockfd))	// has close
		return 0;
		
	int ret = ::close(m_sockfd);
	if(ret == 0x00)
		m_sockfd = -1;
	return ret;
}
int CStreamClient::set_nonblock(bool nonblock)
{
	Pthread::CGuard guard(m_socketmutex);
	if(INVALID_FD(m_sockfd))
		return -1;
		
	return Socket::set_nonblock(m_sockfd, nonblock);
}
int CStreamClient::connect(const std::string &addr, uint16_t port)
{
	Pthread::CGuard guard(m_socketmutex);
	if(INVALID_FD(m_sockfd))
		return 0;

	struct sockaddr_in sockaddr;
	bzero((void *)&sockaddr, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	if(inet_aton(addr.c_str(), &sockaddr.sin_addr) < 0)
		return -1;

	if(::connect(m_sockfd, (struct sockaddr *)(&sockaddr), sizeof(sockaddr)) < 0)
		return -1;
		
	return 0;
}
ssize_t CStreamClient::recv(void *buf, size_t len, int flags)
{
	Pthread::CGuard guard(m_socketmutex);
	if(INVALID_FD(m_sockfd))	
		return -1;
		
	return ::recv(m_sockfd, buf, len, 0);
}
ssize_t CStreamClient::send(const void *buf, size_t len, int flags)
{
	Pthread::CGuard guard(m_socketmutex);
	if(INVALID_FD(m_sockfd))	
		return -1;

	size_t nsend = 0x00;
	ssize_t ret = 0x00;
	do{
		ret = ::send(m_sockfd, (uint8_t *)buf + nsend, len - nsend, 0);
		if(ret < 0x00){
			if(errno != EINTR)
				return ret;
			else
				continue;
		}
		nsend += ret;
	}while(nsend != len);

	return nsend;
}

}