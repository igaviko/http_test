#ifndef _HTTP_SERV_INCLUDED
#define _HTTP_SERV_INCLUDED

#include <stdexcept>
#include <string>
#include <set>
#include <cerrno>
#include <mutex>
#include <functional>
#include <vector>
#include <sstream>
#include <cstdio>

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sock_threads.hpp"



struct HttpServerParam
{
	std::string host;
	std::string dir;
	int prt;
};

class HttpServer
{
	public:
		HttpServer();
		~HttpServer();
		
		HttpServerParam GetServerParams() {return params;}
		void SetServerParams(HttpServerParam &new_params);
		
		void HttpServerStart();		
		void HttpServerStop();		
		void HttpServerLoop();
		
		
	private:
		HttpServer(HttpServer &other) = delete;
		HttpServer& operator=(HttpServer &other) = delete;
		
		HttpServerParam params;
		int lst_sock;
		std::set<int> sockets;
		std::vector<int> close_sockets;
		SockThreads *wrk_threads;

		
		int set_nonblock(int fd);
		void AcceptNewSock();		
		static int ExchangeData(int sock);
		static const std::string index_html;
		static const std::string answ200;
		static const std::string answ404;
										
};

#endif