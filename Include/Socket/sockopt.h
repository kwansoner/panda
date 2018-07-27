/************************************************
* 				sockopt
* 
* desc: socket工具函数
* author: kwanson
* email: CSDN kwanson
*************************************************/

#ifndef __SOCKOPT_H__
#define __SOCKOPT_H__

#include <string>
#include <stdint.h>
#include <netinet/in.h>

namespace Socket
{

// desc: 设置端口复用
// param: sockfd/套接字描述符 en/是否复用
// return: 0/成功返回 -1/错误
int set_resuseport(int sockfd, bool en);

// desc: 设置套接字非阻塞
// param: sockfd/套接字描述符 en/是否非阻塞
// return: 0/成功返回 -1/错误
int set_nonblock(int sockfd, bool en);

// desc: 转换字符串ipv4地址
// param: addr/字符串ipv4地址 port/端口 sockaddr/存放ipv4地址
// return: 0/成功返回 -1/错误
int convert_inaddr(const std::string &addr, uint16_t port, struct sockaddr_in &sockaddr);

}
#endif
