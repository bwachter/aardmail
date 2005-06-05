#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef __WIN32__
#else
#include <sys/wait.h>
#endif
#include "aardmail.h"
#include "aardlog.h"

int am_checkprogram(char *program){
#ifndef __WIN32__
	pid_t pid;
	int status, i;
	char **buf, **bufptr, *ptr;

	if ((buf=malloc(sizeof(char*)*(strlen(program)+2)))==NULL) return -1;

	bufptr=buf;
	*bufptr++=program;
	for (ptr=program;*ptr;ptr++){
		if (*ptr==' '){
			*ptr=0;
			*bufptr++=ptr+1;
		}
	} *bufptr++=NULL;

	if ((pid=fork())==-1){
		logmsg(L_ERROR, F_GENERAL, "fork() failed: ", strerror(errno), NULL);
		return -1;
	}

	if (pid==0){
		execvp(buf[0], buf);
		logmsg(L_DEADLY, F_GENERAL, "execvp() failed: ", strerror(errno), NULL);
	} else {
		free(buf);
		waitpid(pid, &status, 0);
		if (WIFEXITED(status)){
			i=WEXITSTATUS(status);
			if (i==0) return 0;
			else return -1;
		}
	}
	return -1; // should never happen... ;)
#else
	(void) program;
	return 0;
#endif
}

#if (!defined(__WIN32__)) && (!defined _BROKEN_IO)
int am_pipe(char *pipeto){
	int pipefd[2];
	pid_t pid;
	int fd;
	char **buf, **bufptr, *ptr;

	if ((buf=malloc(sizeof(char*)*(strlen(pipeto)+2)))==NULL) return -1;

	bufptr=buf;
	*bufptr++=pipeto;
	for (ptr=pipeto;*ptr;ptr++){
		if (*ptr==' '){
			*ptr=0;
			*bufptr++=ptr+1;
		}
	} *bufptr++=NULL;

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

		execvp(buf[0], buf);
		logmsg(L_DEADLY, F_GENERAL, "execvp() failed: ", strerror(errno), NULL);
	} else {
		free(buf);
		close(pipefd[0]);
		return pipefd[1];
	}
	return -1;
}
#endif

void am_unimplemented(){
	__write2("Sorry, this function is currently unimplemented. Exit.\n");
	exit(0);
}

