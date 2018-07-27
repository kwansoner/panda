/************************************************
* 				sockopt
* 
* desc: socket工具函数
* author: kwanson
* email: CSDN kwanson
*************************************************/

#include <strings.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "sockopt.h"

namespace Socket
{
int set_resuseport(int sockfd, bool en)
{
	int enable = en? 0x01: 0x00;
	return setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (void *)&enable, sizeof(enable));
}
int set_nonblock(int sockfd, bool en)
{
	int flags = fcntl(sockfd, F_GETFL, 0);
	if(en)
		flags |= O_NONBLOCK;
	else
		flags &=~ O_NONBLOCK;
		
	return fcntl(sockfd, F_SETFL, flags);
}
int convert_inaddr(const std::string &addr, uint16_t port, struct sockaddr_in &sockaddr)
{
	bzero((void *)&sockaddr, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	if(inet_aton(addr.c_str(), &sockaddr.sin_addr) < 0)
		return -1;
	return 0;
}
}
