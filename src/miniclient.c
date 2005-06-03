#include <stdlib.h>
#include <stdio.h>
#ifdef __WIN32__
#include <winbase.h>
#else
#include <unistd.h>
#include <sys/poll.h>
#ifndef POLLRDNORM
#warning your system stinks. most likely you should kill drepper.
#define POLLRDNORM 0x0040
#endif
#endif

#include "aardmail.h"
#include "aardlog.h"
#include "authinfo.h"
#include "network.h"
#include "cat.h"

void miniclient_usage(char *program){
	char *tmpstring=NULL;
#ifdef HAVE_SSL
	if (!cat(&tmpstring, "Usage: ", program, " [-c option] -h hostname -s service [-t] [-v level]\n",
#else
	if (!cat(&tmpstring, "Usage: ", program, " -h hostname -s service [-v level]\n",
#endif
#ifdef HAVE_SSL
					 "\t-c:\tcrypto options. Options may be: 0 (off), 1 (tls, like -t),\n",
					 "\t\tand 2 (tls, fallback to plain on error)\n",
#endif
					 "\t-h:\tspecify the hostname to connect to\n",
					 "\t-s:\tthe service to connect to. Must be resolvable if non-numeric.\n",
#ifdef HAVE_SSL
					 "\t-t:\tuse tls. If tls is not possible the program will exit (like -c 1)\n",
#endif
					 "\t-v:\tset the loglevel, valid values are 0 (no logging), 1 (deadly),\n",
					 "\t\t2 (errors, default), 3 (warnings) and 4 (info, very much)\n",
					 NULL)) {
		__write2(tmpstring);
		free(tmpstring);
	}
	exit(0);
}

int main(int argc, char** argv){
	char buf[1024];

#ifdef __WIN32
	// add that windows foo here
#else
	struct pollfd pfd[2];
#endif
	int i, sd;
	authinfo defaultauth;

	memset(&defaultauth, 0, sizeof(authinfo));

	while ((i=getopt(argc, argv, "c:h:s:tv:")) != EOF){
		switch(i){
#ifdef HAVE_SSL
		case 'c':
			switch(atoi(optarg)){
			case 0: am_sslconf=0; break;
			case 1: am_sslconf=AM_SSL_USETLS; break;
			case 2: am_sslconf=AM_SSL_USETLS & AM_SSL_ALLOWPLAIN; break;
			}
			break;
#endif
		case 'h':
			strncpy(defaultauth.machine, optarg, NI_MAXHOST);
			break;
		case 's':
			strncpy(defaultauth.port, optarg, NI_MAXSERV);
			break;
#ifdef HAVE_SSL
		case 't':
			am_sslconf = AM_SSL_USETLS;
			break;
#endif
		case 'v':
			loglevel(atoi(optarg));
			break;
		}
	}

	if (!strcmp(defaultauth.machine,""))
		miniclient_usage(argv[0]);
	if (!strcmp(defaultauth.port, ""))
		miniclient_usage(argv[0]);

	sd=netconnect(defaultauth.machine, defaultauth.port);

#ifdef __WIN32__
#warning miniclient is currently not working under win32
#else
	pfd[0].fd=0;
	pfd[0].events=POLLRDNORM | POLLIN;
	pfd[1].fd=sd;
	pfd[1].events=POLLRDNORM | POLLIN;
 
	i=netreadline(sd, buf);
	__write2(buf);

	while (strcmp(buf, "exit\n")){
		poll(pfd, 2, -1);
		if (pfd[0].revents & (POLLRDNORM | POLLIN)){
			if ((i=read(pfd[0].fd,buf,1024))==-1) return -1;
			buf[i]='\0';
			netwriteline(pfd[1].fd, buf);
		}
		if (pfd[1].revents & (POLLRDNORM | POLLIN)){
			if ((i=netreadline(pfd[1].fd,buf))==-1) return -1;
			__write1(buf);
		}
	}
#endif

	return 0;  
}
