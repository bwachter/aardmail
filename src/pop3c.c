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
#include "authinfo.h"
#include "network.h"
#include "cat.h"
#include "maildir.h"

static struct {
	serverinfo server;
	char *pipeto;
	char *maildir;
	int keepmail;
	int msgcount;
	int onlyget;
	// change everything to fit that variable, or rethink it
	int crypto;
} pop3c;

char *uniqname;
static char *tmpstring;

#ifndef __WIN32__
static int pipefd[2];
#endif

int pop3c_pipe();

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
					 "\t-v:\tset the loglevel, from 0 (no logging) over 2 (default) to 4 (very much)\n",
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

int pop3c_openspool(){
	int fd;

	if (pop3c.pipeto)
		fd=pop3c_pipe();
	else
		fd=maildiropen(NULL, &uniqname);
	if (fd == -1){
		logmsg(L_ERROR, F_GENERAL, "opening spool failed", NULL);
		return -1;
	}
	return fd;
}

int pop3c_closespool(int fd){
	if (pop3c.pipeto){
		close(fd);
		wait(NULL);
	} else
		maildirclose(NULL, &uniqname, fd);
	return 0;
}

long pop3c_getmessage(int sd, int fd, int size){
	(void) size;
	char *tmp;
	char *buf[MAXNETBUF];
	int i, delayrn=0;
	long fsize=0;

	for(;;){
		if ((i=netreadline(sd, (char *)buf)) == -1) {
			logmsg(L_ERROR, F_NET, "unable to read line from network", NULL);
			return -1;
		}
		//fsize+=i;
		tmp=(char *)buf;
		if (!(strcmp((char *)buf, ".\r\n"))){
			write(fd, buf, i-2);
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

int pop3c_connectauth(){
	int i, sd;
	char buf[1024];

	if ((sd=netconnect(pop3c.server.hostname, pop3c.server.service)) == -1)
		return -1;
	if ((i=netreadline(sd, buf)) == -1)
		return -1;
	else logmsg(L_INFO, F_NET, buf, NULL);

#ifdef HAVE_SSL
	// FIXME, don't attempt to continue on error
	if (pop3c.crypto == 3 || pop3c.crypto == 4){ // do we have to use starttls?
		if ((pop3c_oksendline(sd, "stls\r\n")) == -1)
			return -1;

		if ((i=netsslstart(sd))) {
			logmsg(L_ERROR, F_SSL, "unable to open tls-connection using starttls", NULL);
		} else {
			logmsg(L_INFO, F_SSL, "SSL-connection using ", (SSL_get_cipher(ssl)), NULL);
		}
	}
#endif

	if (!cat(&tmpstring, "user ", pop3c.server.username, "\r\n", NULL))
		if ((pop3c_oksendline(sd, tmpstring)) == -1) 
			return -1;

	if (!cat(&tmpstring, "pass ", pop3c.server.password, "\r\n", NULL)){
		if ((pop3c_oksendline(sd, tmpstring)) == -1)
			return -1;
		else return sd;
	}

	return -1; // we should'nt get here...
}

int pop3c_quitclose(int sd){
	if ((pop3c_oksendline(sd, "quit\r\n")) == -1)
		return -1;
	return (close(sd));
}

int pop3c_session(int sd){
	int fd=0, i;
	char *pop3_msgnrbuf[1024];

	if ((pop3c.msgcount=pop3c_getstat(sd)) == -1)
		exit(-1);

	if (pop3c.onlyget == 0 || pop3c.onlyget > pop3c.msgcount) pop3c.onlyget = pop3c.msgcount;

	for (i=1;i<=pop3c.onlyget;i++){
		if (sprintf((char *)pop3_msgnrbuf, "%i", i)==0){
			logmsg(L_ERROR, F_GENERAL, "sprintf() failed", NULL);
			return -1;
		}
		if ((fd=pop3c_openspool())==-1) return -1;
		//pop3c_getsize(sd, i);
		if (!cat(&tmpstring, "retr ", pop3_msgnrbuf, "\r\n", NULL)){
			if ((pop3c_oksendline(sd, tmpstring)) == -1)
				return -1;
			else 
				if (pop3c_getmessage(sd, fd, 0) < 0) goto errclose;
				else {
					if (!pop3c.keepmail) {
						if (cat(&tmpstring, "dele ", pop3_msgnrbuf, "\r\n", NULL))
							goto errclose;
						else 
							if ((pop3c_oksendline(sd, tmpstring)) == -1)
								goto errclose;
					}
				}
		}
		pop3c_closespool(fd);
	}
	return 0;
 errclose:
	pop3c_closespool(fd);
	return -1;
}

int main(int argc, char** argv){
	int c, sd;
	authinfo defaultauth;
	//, pop3c_msgsize=0;
	//char *pop3_msgnrbuf[1024];

	// initialize with sane values
	strcpy(pop3c.server.service, "110");

	pop3c.keepmail = 0;
	pop3c.msgcount = 0;
	pop3c.onlyget = 0;
#ifdef HAVE_SSL
	int starttls=0;
	use_tls = 0;
	allow_plaintext = 0;
#endif

#ifdef HAVE_SSL
	while ((c=getopt(argc, argv, "c:dh:lm:n:p:s:tu:v:x:")) != EOF){
#else
	while ((c=getopt(argc, argv, "dh:m:n:p:s:u:v:x:")) != EOF){
#endif
		switch(c){
#ifdef HAVE_SSL
		case 'c':
			i=atoi(optarg);
			pop3c.crypto = i;
			break;
#endif
		case 'd':
			pop3c.keepmail = 1;
			break;
		case 'h':
			strncpy(pop3c.server.hostname, optarg, NI_MAXHOST);
			break;
#ifdef HAVE_SSL
		case 'l':
			pop3c.crypto=3;
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
			strncpy(pop3c.server.password, optarg, AM_MAXPASS);
			break;
		case 's':
			strncpy(pop3c.server.service, optarg, NI_MAXSERV);
			break;
#ifdef HAVE_SSL
		case 't':
			use_tls=1;
			break;
#endif
		case 'u':
			strncpy(pop3c.server.username, optarg, AM_MAXUSER);
			break;
		case 'v':
			loglevel(atoi(optarg));
			break;
		case 'x':
			pop3c.pipeto = strdup(optarg);
			break;
		}
	}

	if (!strcmp(pop3c.server.hostname,""))
		pop3c_usage(argv[0]);

	//authinfo_init();
	if ((sd=pop3c_connectauth())==-1) exit(-1);
	pop3c_session(sd);
	if (pop3c_quitclose(sd)==-1) exit(-1);
	return 0;  
}
