/************************************************
* 				httpserver
* 
* desc: http server实现
* author: kwanson
* email: CSDN kwanson
*************************************************/

#include <unistd.h>
#include <libgen.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "log.h"
#include "httpserver.h"

namespace Http
{

std::string http_path_handle(const std::string &dname, const std::string &bname)
{
	std::string filepath(HTTP_ROOT);
	if(bname == HTTP_SLASH || bname == HTTP_CURRENT_DIR || \
		bname == HTTP_ABOVE_DIR){	// limit in root dir
		filepath += HTTP_SLASH;
		filepath += HTTP_DEFAULTFILE;
	}else if(dname == HTTP_CURRENT_DIR){
		filepath += HTTP_SLASH;
		filepath += bname;
	}else if(dname == HTTP_SLASH){
		filepath += dname;
		filepath += bname;
	}else{
		filepath += dname;
		filepath += HTTP_SLASH;
		filepath += bname;
	}

	return filepath;
}
void split_url(const std::string &url, std::string &dir, std::string &base)
{
	char *dirc = strdup(url.c_str());
	dir = dirname(dirc);
	delete dirc;
	
	char *basec = strdup(url.c_str());
	base = basename(basec);	
	delete basec;	
}
HttpStream::HttpStream(Socket::IClient *iclient): m_client(iclient)
{
	m_readbuff = new char[READBUFF_LEN + 1];
	assert(m_readbuff != NULL);

	m_client->set_nonblock(true);

	/*
	* 构造最后注册进入epoll
	* 防止未构造完全收到事件回调
	*/
	register_event(*m_client);
	trace("constructor socket[%#X]\n", m_client->fd());
}
HttpStream::~HttpStream()
{
	trace("distructor socket[%#X]\n", m_client->fd());

	delete m_client;
	m_client = NULL;
	
	delete m_readbuff;
	m_readbuff = NULL;
}
int HttpStream::close()
{
	return shutdown_event(*m_client);
}
void HttpStream::handle_in(int fd)
{
	Pthread::CGuard guard(m_readbuffmutex);
	ssize_t nread = m_client->recv(m_readbuff, READBUFF_LEN, MSG_DONTWAIT);
	if((nread < 0 && nread != EAGAIN) || nread == 0){	// error or read EOF
		close();
		return;
	}else if(nread < 0 && nread == EAGAIN){ 			
		errsys("non data to read\n");
		return; 
	}	
	m_readbuff[nread] = 0x00;

	Http::CHttpRequest httprequest;
	if(httprequest.load_packet(m_readbuff, nread) < 0){
		error("parse package error\n");
		return;
	}

	trace("socket[%d] receive <--- %ld bytes\n", fd, nread);
	Http::IHttpRespose *respose = handle_request(httprequest);
	if(respose != NULL){
		m_client->send(respose->serialize(), respose->size(), 0);
		delete respose;
	}
}
void HttpStream::handle_out(int fd)
{
	
}
void HttpStream::handle_close(int fd)
{
	trace("socket[%d] handle close\n", fd);
	delete this;
}
void HttpStream::handle_error(int fd)
{
	close();
}
Http::IHttpRespose *HttpStream::handle_request(Http::IHttpRequest &request)
{
	const std::string &method = request.method();
	const std::string &url = request.url();

	std::string dname, bname;
	split_url(url, dname, bname);

	Http::CHttpRespose *respose = new Http::CHttpRespose;	
	std::string filepath = http_path_handle(dname, bname);
	if(method == "GET"){

		std::string extention = extention_name(filepath);
		if(extention.empty() || access(filepath.c_str(), R_OK) < 0){

			errsys("access %s error\n", filepath.c_str());
			
			respose->set_version(HTTP_VERSION);
			respose->set_status("404", "Not Found");
			respose->add_head(HTTP_HEAD_CONNECTION, "close");
			return respose;
		}

		
		struct stat filestat;
		stat(filepath.c_str(), &filestat);
		const size_t filesize = filestat.st_size;

		char *fbuff = new char[filesize];
		assert(fbuff != NULL);

		FILE *fp = fopen(filepath.c_str(), "rb");
		if(fp == NULL || fread(fbuff, filesize, 1, fp) != 0x01){
		
			delete fbuff;

			respose->set_version(HTTP_VERSION);
			respose->set_status("500", "Internal Server Error");
			respose->add_head(HTTP_HEAD_CONNECTION, "close");
			return respose;
		}
		
		fclose(fp);

		char sfilesize[16] = {0x00};
		snprintf(sfilesize, sizeof(sfilesize), "%ld", filesize);

		respose->set_version(HTTP_VERSION);
		respose->set_status("200", "OK");
		respose->add_head(HTTP_HEAD_CONTENT_TYPE, http_content_type(extention));
		respose->add_head(HTTP_HEAD_CONTENT_LEN, sfilesize);
		respose->add_head(HTTP_HEAD_CONNECTION, "close");
		respose->set_body(fbuff, filesize);
		delete fbuff;
	}

	return respose;
}

int HttpServer::start(size_t backlog)
{
	if(!m_server.isclose())	 //has start
		return 0;
		
	if(m_server.start(backlog) < 0)
		return -1;

	if(m_server.set_nonblock(true) < 0)
		errsys("set socket non block failed\n");

	return register_event(m_server);
}
int HttpServer::close()
{
	return shutdown_event(m_server);
}
HttpServer::HttpServer(const std::string &addr, uint16_t port):
	m_port(port), m_addr(addr), m_server(addr, port)
{
	trace("constructor ip %s, port %d\n", addr.c_str(), port);
}
HttpServer::~HttpServer()
{
	trace("distructor ip %s, port %d\n", m_addr.c_str(), m_port);
}
void HttpServer::handle_in(int fd)
{
	/*
	* 读取所有建立的连接
	*/
	do{
		Socket::IClient *newconn = m_server.accept();
		if(newconn == NULL){
			if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
				break;
			}else{
				errsys("sockfd[%d] accept error\n", fd);
				close();
				return;
			}
		}
		
		trace("socket[%d] accept a conn\n", fd);
		HttpStream *httpstream = new HttpStream(newconn);	// self release 
		assert(httpstream != NULL);

	}while(true);
}
void HttpServer::handle_out(int fd)
{
}
void HttpServer::handle_close(int fd)
{
	trace("socket[%d] handle close\n", fd);
}
void HttpServer::handle_error(int fd)
{	
	close();
}
}

