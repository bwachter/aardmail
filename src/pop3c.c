#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#ifdef __WIN32__
#else
#include <sys/wait.h>
#endif

#include "aardmail.h"
#include "aardlog.h"
#include "network.h"
#include "cat.h"
#include "maildir.h"

struct {
	char *username;
	char *password;
	char *hostname;
	char *service;
	char *pipeto;
	char *maildir;
	int keepmail;
	int msgcount;
	int onlyget;
} pop3c;

char *uniqname;
#ifndef __WIN32__
static int pipefd[2];
#endif

void pop3c_usage(char *program){
	char *tmpstring=NULL;
#ifndef HAVE_SSL
	if (!cat(&tmpstring, "Usage: ", program, " [-d] -h hostname [-m maildir] [-p password]\n",
					 "\t\t[-s service] [-u user] [-x program]","\n",
#else
	if (!cat(&tmpstring, "Usage: ", program, " [-c option] [-d] -h hostname [-l] [-m maildir] [-p password]\n",
					 "\t\t[-s service] [-t] [-u user] [-x program]","\n",
					 "\t-c:\tcrypto options. Options may be: 0 (off), 1 (tls, like -t),\n",
					 "\t\t 2 (tls, fallback to plain on error), 3 (starttls, no fallback) and\n",
					 "\t\t 4 (starttls, fallback to plain on error)\n",
#endif
					 "\t-d:\tdon't delete mail after retrieval (default is to delete)\n",
					 "\t-h:\tspecify the hostname to connect to\n",
#ifdef HAVE_SSL
					 "\t-l:\tuse starttls, exit on error (like -c 3)\n",
#endif
					 "\t-m:\tthe maildir for spooling; default (unless -x used) is ~/Maildir\n",
					 "\t-n:\tonly load number mails\n",
					 "\t-p:\tthe password to use. Don't use this option.\n",
					 "\t-s:\tthe service to connect to. Must be resolvable if non-numeric.\n",
#ifdef HAVE_SSL
					 "\t-t:\tuse tls. If tls is not possible the program will exit (like -c 1)\n",
#endif
					 "\t-u:\tthe username to use. You usually don't need this option.\n",
					 "\t-x:\tthe program to popen() for each received mail\n",
					 NULL)) {
		__write2(tmpstring);
		free(tmpstring);
	}
	exit(0);
}

void pop3c_unimplemented(){
	__write2("Sorry, this function is currently unimplemented. Exit.\n");
	exit(0);
}

int pop3c_oksendline(int sd, char *msg){
	char buf[MAXNETBUF];
	int i;

	logmsg(L_INFO, F_NET, "> ", msg, NULL);
	if ((i=netwriteline(sd, msg)) == -1){
		logmsg(L_ERROR, F_NET, "unable to write line to network: ", strerror(errno), NULL);
		return -1;
	}
	if ((i=netreadline(sd, buf)) == -1){
		logmsg(L_ERROR, F_NET, "unable to read line from network: ", strerror(errno), NULL);
		return -1;
	}
	logmsg(L_INFO, F_NET, "< ", buf, NULL);
	if (!strncmp(buf, "+OK", 3))
		return 0;
	logmsg(L_ERROR, F_NET, "bad response: '", buf, "' after '", msg, "' from me", NULL); 
	return -1;
}

int pop3c_getstat(int sd){
	char *tmp, *tmp2;
	char buf[MAXNETBUF];
	int i;

	logmsg(L_INFO, F_NET, "> ", "stat", NULL);
	if ((i=netwriteline(sd,"stat\r\n")) == -1){
		logmsg(L_ERROR, F_NET, "unable to write line to network: ", strerror(errno), NULL);
		return -1;
	}
	if ((i=netreadline(sd, buf)) == -1){
		logmsg(L_ERROR, F_NET, "unable to read line from network: ", strerror(errno), NULL);
		return -1;
	}
	logmsg(L_INFO, F_NET, "< ", buf, NULL);
	if (strncmp(buf, "+OK", 3)){
		logmsg(L_ERROR, F_NET, "bad response: '", buf, "'after 'stat' from me", NULL);
		return -1;
	}

	for (tmp=buf+4,i=0;*tmp!=' ';*tmp++,i++){}
	if((tmp2=malloc((i+1)*sizeof(char))) == NULL){
		logmsg(L_ERROR, F_GENERAL, "malloc() failed at pop3c_getstat()", NULL);
		return -1;
	}
	strncpy(tmp2, buf+4, i);
	logmsg(L_INFO, F_NET, "There are ", tmp2, " messages on server", NULL);
	return atoi(tmp2);
}

int pop3c_getsize(int sd, int msgnr){
	char msgnrbuf[1024];
	char *command=NULL, *tmp=NULL;
	char buf[MAXNETBUF];
	int i;

	sprintf(msgnrbuf, "%i", msgnr);
	if ((cat(&command, "list ", msgnrbuf, "\r\n", NULL))) return -1;
	logmsg(L_INFO, F_NET, "> ", tmp, NULL);
	if ((i=netwriteline(sd,command)) == -1){
		logmsg(L_ERROR, F_NET, "unable to write line to network: ", strerror(errno), NULL);
		free(command);
		return -1;
	}
	if ((i=netreadline(sd, buf)) == -1){
		logmsg(L_ERROR, F_NET, "unable to read line from network: ", strerror(errno), NULL);
		free(command);
		return -1;
	}
	logmsg(L_INFO, F_NET, "< ", buf, NULL);
	if (strncmp(buf, "+OK", 3)){
		logmsg(L_ERROR, F_NET, "bad response: '", buf, "'after '", command, "' from me", NULL);
		free(command);
		return -1;
	}
	/*
		for (tmp=buf+4,i=0;*tmp!=' ';*tmp++,i++){}
		if((tmp2=malloc((i+1)*sizeof(char))) == NULL){
		logmsg(L_ERROR, F_GENERAL, "malloc() failed at pop3c_getstat()", NULL);
		return -1;
		}
		strncpy(tmp2, buf+4, i);
		logmsg(L_INFO, F_NET, "There are ", tmp2, " messages on server", NULL);
		return atoi(tmp2);
	*/
	free(command);
	return 0;
}

#ifdef __WIN32__

long pop3c_getmessage(int sd, int fd, int msgnr, int size){
	(void) msgnr;
	(void) size;
	char *tmp;
	char *buf[MAXNETBUF];
	int i, delayrn=0;
	long fsize=0;

	for(;;){
		if ((i=netreadline(sd, (char *)buf)) == -1) {
			logmsg(L_ERROR, F_NET, "unable to read line from network", NULL);
			close(fd);
		}
		//fsize+=i;
		tmp=(char *)buf;
		if (!(strcmp((char *)buf, ".\r\n"))){
			write(fd, buf, strlen((char *)buf)-3);
			if (pop3c.pipeto) pclose(fd);
			else {
				if (pop3c.pipeto)
					fclose(fd);
				else
					maildirclose(NULL, &uniqname, fd);
			}
			return(fsize);
		} else {
			if (delayrn){
				write(fd, "\r\n", 2); 
				delayrn=0;
			}
			// we'll delay writing \r\n till we get here the next time in case of \r\n.\r\n as ending
			if (!strcmp((char *)buf, "\r\n")) delayrn=1;
			else {
				if (!strncmp((char *)buf, ".", 1)){
					tmp=(char *)buf;
					*tmp++;
					write(fd, tmp, strlen((char *)buf)-1);
				} else
					write(fd, buf, strlen((char *)buf));
			}
		}
	}
}
#else
long pop3c_getmessage(int sd, int fd, int msgnr, int size){
	(void) msgnr;
	(void) size;
	char *tmp;
	char *buf[MAXNETBUF];
	int i, delayrn=0;
	long fsize=0;

	for(;;){
		if ((i=netreadline(sd, (char *)buf)) == -1) {
			logmsg(L_ERROR, F_NET, "unable to read line from network", NULL);
			close(fd);
		}
		//fsize+=i;
		tmp=(char *)buf;
		if (!(strcmp((char *)buf, ".\r\n"))){
			write(fd, buf, i-2);
			if (pop3c.pipeto){
					close(fd);
					wait(NULL);
			} else
				maildirclose(NULL, &uniqname, fd);
			return(fsize);
		} else {
			if (delayrn){
				write(fd, "\n", 1); 
				delayrn=0;
			}
			// we'll delay writing \r\n till we get here the next time in case of \r\n.\r\n as ending
			if (!strcmp((char *)buf, "\r\n")) delayrn=1;
			else {
				tmp=(char *)buf;
				if (!strncmp(tmp+i-1, "\r\n", 2)){
					strncpy(tmp+i-1, "\n", 1);
					i--;
				}
				if (!strncmp((char *)buf, ".", 1)){
					tmp=(char *)buf;
					*tmp++;
					write(fd, tmp, i);
				} else
					write(fd, buf, i+1);
			}
		}
	}
}

int pop3c_pipe(){
	pid_t pid;
	int fd;
	if (pipe(pipefd)==-1){
		logmsg(L_ERROR, F_GENERAL, "pipe() failed: ", strerror(errno), NULL);
		return -1;
	}

		if ((pid=fork())==-1){
			logmsg(L_ERROR, F_GENERAL, "fork() failed: ", strerror(errno), NULL);
			return -1;
		}

		if (pid == 0){
			close(0);
			fd=dup(pipefd[0]);
			close(pipefd[0]);
			close(pipefd[1]);
			execlp("/usr/bin/procmail", "procmail", 0);
			logmsg(L_DEADLY, F_GENERAL, "execlp() failed: ", strerror(errno), NULL);
		} else {
			close(pipefd[0]);
			return pipefd[1];
		}
		return -1;
}
#endif

int pop3c_openspoolfile(int msgnr){
	char msgnrbuf[1024];
	int fd;

	sprintf(msgnrbuf, "%i", msgnr);
	fd=maildiropen(NULL, &uniqname);
	//logmsg(L_INFO, F_GENERAL, pop3c.maildir, uniqname, NULL);
	return (fd);
}

int pop3c_spool2pipe(){
	return 0;
}

int main(int argc, char** argv){
	char *tmpstring=NULL;
	char buf[1024];
	int i, sd, c, fd;
	//, pop3c_msgsize=0;
	char *pop3_msgnrbuf[1024];

	pop3c.keepmail = 0;
	pop3c.msgcount = 0;
	pop3c.onlyget = 0;
#ifdef HAVE_SSL
	int starttls=0;
	use_tls = 0;
	allow_plaintext = 0;
#endif

#ifdef HAVE_SSL
	while ((c=getopt(argc, argv, "c:dh:lm:n:p:s:tu:x:")) != EOF){
#else
	while ((c=getopt(argc, argv, "dh:m:n:p:s:u:x:")) != EOF){
#endif
		switch(c){
#ifdef HAVE_SSL
		case 'c':
			i=atoi(optarg);
			switch(i){
			case 0:
				use_tls = 0;
				allow_plaintext = 0;
				starttls = 0;
				break;
			case 1:
				use_tls = 1;
				allow_plaintext = 0;
				starttls = 0;
				break;
			case 2:
				use_tls = 1;
				allow_plaintext = 1;
				starttls = 0;
				break;
			case 3:
				use_tls = 0; // we'll use plaintext first, then STLS
				allow_plaintext = 0;
				starttls = 1;
				break;
			case 4:
				use_tls = 0;
				allow_plaintext = 1;
				starttls = 1;
				break;
			default:
				pop3c_usage(argv[0]);
			}
			break;
#endif
		case 'd':
			pop3c.keepmail = 1;
			break;
		case 'h':
			if ((pop3c.hostname = strdup(optarg)) == NULL)
				pop3c_usage(argv[0]);
			break;
#ifdef HAVE_SSL
		case 'l':
			use_tls=0;
			starttls=1;
			break;
#endif
		case 'm':
			pop3c.maildir = strdup(optarg);
			break;
		case 'n':
			pop3c.onlyget = atoi(optarg);
			break;
		case 'p':
			logmsg(L_WARNING, F_GENERAL, "do not use -p password, it's unsecure. use .authinfo", NULL);
			pop3c.password = strdup(optarg);
			break;
		case 's':
			pop3c.service = strdup(optarg);
			break;
#ifdef HAVE_SSL
		case 't':
			use_tls=1;
			break;
#endif
		case 'u':
			pop3c.username = strdup(optarg);
			break;
		case 'x':
			pop3c.pipeto = strdup(optarg);
			break;
		}
	}

	if (!pop3c.hostname)
		pop3c_usage(argv[0]);

	if (!pop3c.service)
		pop3c.service=strdup("110");

	if ((sd=netconnect(pop3c.hostname, pop3c.service)) == -1)
		exit(-1);

	if ((i=netreadline(sd, buf)) == -1)
		exit(-1);
	else
		logmsg(L_INFO, F_NET, buf, NULL);

#ifdef HAVE_SSL
	if (starttls){
		if ((pop3c_oksendline(sd, "stls\r\n")) == -1)
			exit (-1);

		if ((i=netsslstart(sd))) {
			logmsg(L_ERROR, F_SSL, "unable to open tls-connection using starttls", NULL);
		} else {
			logmsg(L_INFO, F_SSL, "SSL-connection using ", (SSL_get_cipher(ssl)), NULL);
		}
	}
#endif

	if (!cat(&tmpstring, "user ", pop3c.username, "\r\n", NULL))
		if ((pop3c_oksendline(sd, tmpstring)) == -1) 
			exit(-1);

	if (!cat(&tmpstring, "pass ", pop3c.password, "\r\n", NULL))
		if ((pop3c_oksendline(sd, tmpstring)) == -1)
			exit (-1);

	if ((pop3c.msgcount=pop3c_getstat(sd)) == -1)
		exit(-1);

	if (pop3c.onlyget == 0 || pop3c.onlyget > pop3c.msgcount) pop3c.onlyget = pop3c.msgcount;

	for (i=1;i<=pop3c.onlyget;i++){
		if (pop3c.pipeto)
			fd=pop3c_pipe();
		else
			fd=pop3c_openspoolfile(i);
		if (fd != -1){
			if (sprintf((char *)pop3_msgnrbuf, "%i", i)==0)
				logmsg(L_ERROR, F_SSL, "problem with sprintf()", NULL);
			else {
				//pop3c_getsize(sd, i);
				if (!cat(&tmpstring, "retr ", pop3_msgnrbuf, "\r\n", NULL)){
					if ((pop3c_oksendline(sd, tmpstring)) == -1)
						exit (-1);
					else 
						if (pop3c_getmessage(sd, fd, i, 0) >= 0){
							if (!pop3c.keepmail){
								if (!cat(&tmpstring, "dele ", pop3_msgnrbuf, "\r\n", NULL))
									if ((pop3c_oksendline(sd, tmpstring)) == -1)
										exit (-1);
							}
						}
				}
			}
		}
	}
	if ((pop3c_oksendline(sd, "quit\r\n")) == -1)
		exit (-1);

	return 0;  
}
