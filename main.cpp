#include <iostream>

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>

#include "http_serv.hpp"


extern char *optarg;
extern int optind, opterr, optopt;

void demonize(const char* dir)
{
	pid_t pid;

    if ((pid = fork()) == -1)
        exit(EXIT_FAILURE);

    if (pid > 0)	// Child
        exit(EXIT_SUCCESS);

    if (setsid() == -1)
        exit(EXIT_FAILURE);

    struct sigaction sa_chld, sa_hup;
	
	// ignore SIGCHLD, SIGHUP
	sigfillset(&sa_chld.sa_mask);
    sa_chld.sa_handler = SIG_IGN;
    sa_chld.sa_flags = 0;
    errno = 0;
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1)
    	exit(EXIT_FAILURE);
	
	sigfillset(&sa_hup.sa_mask);
    sa_hup.sa_handler = SIG_IGN;
    sa_hup.sa_flags = 0;
    errno = 0;
    if (sigaction(SIGHUP, &sa_hup, NULL) == -1)
    	exit(EXIT_FAILURE);

    if ((pid = fork()) < 0)
        exit(EXIT_FAILURE);

    if (pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    if (strcmp (dir,"") == 0)
		chdir("/");
	else
		chdir(dir);
}


int main(int argc, char* argv[])
{
	HttpServerParam prm;
	opterr=0;
	int rez = 0;
	while ((rez = getopt(argc, argv, "h:p:d:")) != -1)
	{
		switch(rez)
		{
			case 'h':
				//std::cout << "Host = " << optarg << std::endl;
				prm.host = optarg;
				break;
			case 'p':
				//std::cout << "Port = " << optarg << std::endl;
				prm.prt = atoi(optarg);
				break;
			case 'd':
				//std::cout << "Directory = " << optarg << std::endl;
				prm.dir = optarg; 
				break;			
		}
	}
	
	demonize(prm.dir.c_str());
	
	try
	{
		HttpServer server;
		server.SetServerParams(prm);
		server.HttpServerStart();
		while (true)
			server.HttpServerLoop();
	}
	catch(std::exception &e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
	
	return 0;
}