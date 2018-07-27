/************************************************
* 				ISocket
* 
* desc: socket接口类
* author: kwanson 2018/7/21
* email: CSDN kwanson
*************************************************/

#ifndef __ISOCKET_H__
#define __ISOCKET_H__

#include <stdint.h>
#include <sys/types.h>

#include <string>

namespace Socket
{
class ISocket
{
	public:
		virtual ~ISocket(){};
		
		// desc: 获取套接字描述符
		// param: void
		// return: 套接字描述符
		virtual int fd() = 0;
};

class IClient: public ISocket
{
	public:
		virtual ~IClient(){};

		// desc: 打开套接字
		// param: void
		// return: 0/成功 -1/失败
		virtual int start() = 0;

		// desc: 关闭套接字
		// param: void
		// return: 0/成功 -1/失败
		virtual int close() = 0;

		// desc: 设置套接字为非阻塞
		// param: block/是否非阻塞
		// return: 0/成功 -1/失败
		virtual int set_nonblock(bool nonblock) = 0;

		// desc: 连接到server
		// param: addr/server地址 port/端口
		// return: 0/成功 -1/失败
		virtual int connect(const std::string &addr, uint16_t port) = 0;

		// desc: 读取数据
		// param: buf/存放接收数据缓冲区 len/buf长度 flag/见man recv
		// return: 读取数据长度
		virtual ssize_t recv(void *buf, size_t len, int flags) = 0;

		// desc: 发送数据
		// param: buf/存放发送数据缓冲区 len/buf长度 flag/见man recv
		// return: 发送数据长度
		virtual ssize_t send(const void *buf, size_t len, int flags) = 0;
};


class IServer: public ISocket
{
	public: 
		virtual ~IServer(){};	

		// desc: 设置套接字为非阻塞
		// param: block/是否非阻塞
		// return: 0/成功 -1/失败
		virtual int set_nonblock(bool block) = 0;

		// desc: 打开套接字
		// param: backlog/内核连接队列最大长度
		// return: 0/成功 -1/失败
		virtual int start(size_t backlog) = 0;

		// desc: 关闭套接字
		// param: void
		// return: 0/成功 -1/失败
		virtual int close() = 0;

		// desc: 套接字是否关闭
		// param: void
		// return: true/关闭 false/未关闭
		virtual bool isclose() = 0;

		// desc: 返回一个新的连接, 需要手动释放内存
		// param: void
		// return: NULL/错误    NOT NULL/新的连接
		virtual IClient *accept() = 0;
};

}
#endif
