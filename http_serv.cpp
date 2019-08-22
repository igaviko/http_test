#include <iostream>
#include "http_serv.hpp"

const std::string HttpServer::index_html = "<HTML><TITLE>Hello!</TITLE><BODY><P>Test page</P></BODY></HTML>";
/*			
const std::string HttpServer::answ200 = 	"HTTP/1.0 200 OK\r\n"
			"Content-length: %d\r\n"
			"Content-Type: text/html\r\n"
			"\r\n%s";		
*/
const std::string HttpServer::answ200 = 	"HTTP/1.0 200 OK\r\n"
			"Content-length: 63%d\r\n"
			"Content-Type: text/html\r\n"
			"\r\n<HTML><TITLE>Hello!</TITLE><BODY><P>Test page</P></BODY></HTML>";
			
const std::string HttpServer::answ404 = 	"HTTP/1.0 404 NOT FOUND\r\n"
						"Content-length: 0\r\n"
						"Content-Type: text/html\r\n"
						"\r\n";

HttpServer::HttpServer()
{
	params.host = "127.0.0.1";
	params.prt = 80;
	params.dir = "web";
	lst_sock = -1;

	wrk_threads = new SockThreads(&HttpServer::ExchangeData/*, mut*/, close_sockets);
}

HttpServer::~HttpServer() 
{	
	if (lst_sock != -1)
	{
		shutdown(lst_sock, SHUT_RDWR);
		close(lst_sock);
	}
	
	if (wrk_threads != nullptr)
		delete wrk_threads;
}

void HttpServer::SetServerParams(HttpServerParam &new_params)
{
	if (new_params.host == "")
		throw std::runtime_error("Bad host");
	
	if (new_params.prt < 0)
		throw std::runtime_error("Bad port");
	
	params = new_params; 
}

void HttpServer::HttpServerStart() 
{
	sockets.clear();
	errno = 0;
	lst_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (lst_sock == -1) 
		throw std::runtime_error(std::strerror(errno));
	
	set_nonblock(lst_sock);
	
	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(params.prt);
	inet_pton(AF_INET, params.host.c_str(), &sock_addr.sin_addr);
	
	int ret = bind(lst_sock, reinterpret_cast<struct sockaddr *>(&sock_addr), sizeof(struct sockaddr_in));
	if (ret == -1)
		throw std::runtime_error(std::strerror(errno));
	
	ret = listen(lst_sock, SOMAXCONN);
	if (ret == -1)
		throw std::runtime_error(std::strerror(errno));
}

void HttpServer::HttpServerStop()
{
	if (lst_sock != 0)
	{
		shutdown(lst_sock, SHUT_RDWR);
		close(lst_sock);
		lst_sock = 0;
	}
}

void HttpServer::HttpServerLoop()
{
	errno = 0;
	fd_set set;
	FD_ZERO(&set);
	FD_SET(lst_sock, &set);
	
	for(auto iter = sockets.begin(); iter != sockets.end();)
	{
		bool ok = true;
		for (auto val : close_sockets)
		{
			if (val == *iter)
			{
				ok = false;
				break;
			}
		}
		
		if (!ok)
			iter = sockets.erase(iter);
		else
		{
			FD_SET(*iter, &set);
			++iter;
		}
	}
	close_sockets.clear();
	
	int max = std::max(lst_sock, (sockets.empty() ? -1 : *sockets.rbegin()));
	int res = select(max+1, &set, NULL, NULL, NULL);
	if (res == -1)
		throw std::runtime_error(std::strerror(errno));
	
	if (FD_ISSET (lst_sock, &set))
		AcceptNewSock();
	
	for(auto sock_it = sockets.begin(); sock_it != sockets.end(); ++sock_it)
	{
		if (FD_ISSET (*sock_it, &set))
			wrk_threads->AddSock(*sock_it);
	}
	wrk_threads->WaitWrk();
}
	
int HttpServer::set_nonblock(int fd)
{
	int flags = 0;
	#if defined(O_NONBLOCK)
		if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
			flags = 0;
		return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	#else
		flags = 1;
		return ioctl(fd, FIOBIO, &flags);
	#endif
}

void HttpServer::AcceptNewSock()
{
	int new_sock = accept(lst_sock, 0, 0);
	if (new_sock == -1)
		throw std::runtime_error(std::strerror(errno));
	set_nonblock(new_sock);
	sockets.insert(new_sock);
}

int HttpServer::ExchangeData(int sock)
{
	int ret = 0;
	char *rbuff = new char[1024];
	char *sbuff = new char[1024];
	errno = 0;
	int rcv_sz = recv(sock, rbuff, 1024, MSG_NOSIGNAL);
	/*
	if ((rcv_sz <= 0) && (errno != EAGAIN)) 
	{
		//shutdown(sock, SHUT_RDWR);
		//close(sock);
		ret = -1;
	} 
	else */
	if (rcv_sz > 0) 
	{
		if ((rcv_sz < 3)&&(rbuff[0] == 'q'))
		{
			//shutdown(sock, SHUT_RDWR);
			//close(sock);
			ret = -1;
		}
		else
		{
			int n = 0;
			std::istringstream in_buff(rbuff);
			std::string tmp;
			in_buff >> tmp;
			if (tmp != "GET")
				n = sprintf(sbuff, "Bad reqest");
			else
			{
				in_buff >> tmp;
				auto tmp1 = tmp.substr(0,tmp.find('?'));
				if ((tmp1 == "/")||(tmp1 == "/index.html"))
					n = sprintf(sbuff, "%s", answ200.c_str()/*, index_html.length(), index_html.c_str()*/);
				else
					n = sprintf(sbuff, "%s", answ404.c_str());
			}
			send(sock, sbuff, n, MSG_NOSIGNAL);
		}
	}
	
	shutdown(sock, SHUT_RDWR);
	close(sock);
	
	delete [] rbuff;
	delete [] sbuff;
	
	return -1;
}
