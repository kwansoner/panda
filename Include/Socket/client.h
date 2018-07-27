/************************************************
* 				client
* 
* desc: socket client声明
* author: kwanson
* email: CSDN kwanson
*************************************************/

#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <stdint.h>
#include <sys/types.h>

#include <string>

#include "mutex.h"
#include "ISocket.h"

namespace Socket
{
class CStreamClient: public IClient
{
	public:
		CStreamClient();
		CStreamClient(int fd);
		CStreamClient(const CStreamClient &streamclient);
		~CStreamClient();
		int fd();
		int start();
		int close();
		int set_nonblock(bool nonblock);
		int connect(const std::string &addr, uint16_t port);
		ssize_t recv(void *buf, size_t len, int flags = 0);
		ssize_t send(const void *buf, size_t len, int flags = 0);
	private:
		int m_sockfd;
		Pthread::CMutex m_socketmutex;
};
}

#endif