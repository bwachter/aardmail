#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#ifdef __WIN32__
#include <windows.h>
#include <winbase.h>
#include <io.h>
#include <ws2tcpip.h>
#else 
#include <unistd.h>
#include <netdb.h>
#endif

#include "cat.h"
#include "aardlog.h"
#include "maildir.h"

static int deliveries=0;

int maildirgname(char **uniqname){
	char tmpbuf[512];
	char *myhost[NI_MAXHOST];
	int myhost_len=0;
	time_t mytime=time(NULL);
	pid_t mypid=getpid();

	gethostname((char *)myhost, myhost_len);

	deliveries++;
	sprintf(tmpbuf, "%li.P%iQ%i", (unsigned long)mytime, (int) mypid, deliveries);
#ifdef _POSIX_SOURCE

#endif
	cat(&*uniqname, tmpbuf, ".", myhost, NULL);
	return 0; // FIXME, kludge
}

int maildirfind(char *maildir){
	// TODO: maybe look what's in maildirpath. check if directory exists
	if (maildir) maildirpath=strdup(maildir);
	else{
		if ((maildirpath=getenv("MAILDIR"))==NULL){
			if (getenv("HOME")==NULL){
				logmsg(L_ERROR, F_GENERAL, "$MAILDIR not set, $HOME not found", NULL);
				return -1;
			} else {
				if (cat(&maildirpath, getenv("HOME"), "/Maildir", NULL)) return -1;
				else return 0;
			}
		} else {
			maildirpath=strdup(getenv("MAILDIR"));
			return 0;
		}
	}
	return 0;//FIXME
}

// opens a file in maildir/ 
int maildiropen(char *maildir, char **uniqname){
	char *path=NULL;
	int fd;

	if ((maildirfind(maildir)) == -1) return -1;

	maildirgname(uniqname);
	if ((cat(&path, maildirpath, "/new/", *uniqname, NULL))) return -1;
	logmsg(L_INFO, F_GENERAL, "spooling to ", path, NULL);
	if ((fd=open(path, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0644)) == -1) {
		logmsg(L_ERROR, F_GENERAL, "open ", path, " for writing failed: ", strerror(errno), NULL);
		free(path);
		return -1;
	}
	free(path);
	return fd;
}

int maildirclose(char *maildir, char **uniqname, int fd){
	char *oldpath=NULL, *newpath=NULL;
	int status=0;
	if ((maildirfind(maildir)) == -1) {
		close(fd);
		return -1;
	}

	cat(&oldpath, maildirpath, "/new/", *uniqname, NULL);
	cat(&newpath, maildirpath, "/cur/", *uniqname, ":2,", NULL);

	if (!close(fd)){
#ifdef __WIN32__
		status=MoveFile(oldpath, newpath);
#else
		status=link(oldpath, newpath);
		unlink(oldpath);
#endif
	}

	free(oldpath);
	free(newpath);
	return status;
}

