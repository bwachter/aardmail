#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <dirent.h>
#ifdef __WIN32__
#include <windows.h>
#include <winbase.h>
#include <io.h>
#include <ws2tcpip.h>
#include <process.h>
#else 
#include <unistd.h>
#include <netdb.h>
#endif
#ifdef _POSIX_SOURCE
#include <sys/time.h>
#endif

#include <ibaard_network.h>
#include <ibaard_log.h>
#include <ibaard_fs.h>
#include <ibaard_cat.h>

#include "maildir.h"
// for maildirformat look at http://cr.yp.to/proto/maildir.html
static int deliveries=0;
static maildirent *maildir_storage;
static int maildir_cnt=0;
static int maildir_harddelete=0; // shall we delete the mails, or just mark them deleted and remove them on cleanup?

static int maildir_sappend(maildirent *maildir_add);

static int maildirgname(char **uniqname){
	char tmpbuf[512];
	char myhost[NI_MAXHOST];
	pid_t mypid=getpid();
#ifndef _POSIX_SOURCE
	time_t mytime=time(NULL);
#else
	struct timeval mytime;
	gettimeofday(&mytime, NULL);
#endif

	if (gethostname(myhost, NI_MAXHOST)==-1){
		logmsg(L_WARNING, F_MAILDIR, "unable to get hostname, setting to localhost.localdomain", NULL);
		strcpy(myhost, "localhost.localdomain");
	}

	deliveries++;

#ifdef _POSIX_SOURCE
	sprintf(tmpbuf, "%li.M%liP%iQ%i", (unsigned long)mytime.tv_sec, (unsigned long)mytime.tv_usec, (int) mypid, deliveries);
#else
	sprintf(tmpbuf, "%li.P%iQ%i", (unsigned long)mytime, (int) mypid, deliveries);
#endif
	cat(&*uniqname, tmpbuf, ".", myhost, NULL);

	return 0; // FIXME, kludge
}

int maildirfind(char *maildir){
	// TODO: maybe look what's in maildirpath. check if directory exists
	if (maildir) {
		maildirpath=strdup(maildir);
		if (!td(maildirpath)) return 0; 
	} 

	if ((maildirpath=getenv("MAILDIR"))==NULL){
#ifdef __WIN32__
		if (getenv("USERPROFILE")==NULL){
			logmsg(L_ERROR, F_MAILDIR, "%MAILDIR% not set, %USERPROFILE% not found", NULL);
#else
		if (getenv("HOME")==NULL){
			logmsg(L_ERROR, F_MAILDIR, "$MAILDIR not set, $HOME not found", NULL);
#endif
			return -1;
		} else {
#ifdef __WIN32__
			if (cat(&maildirpath, getenv("USERPROFILE"), "/Maildir", NULL)) return -1;
#else
			if (cat(&maildirpath, getenv("HOME"), "/Maildir", NULL)) return -1;
#endif
			else if (!td(maildirpath)) return 0;
		}
	} else {
		maildirpath=strdup(getenv("MAILDIR"));
		if (!td(maildirpath)) return 0;
	}

	return -1; // if we got that far we did not find a usable maildir
}

// opens a file in maildir/ 
// if maildir does not start with a / use subdir in maildir
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
FILE *maildiropen(char *maildir, char **uniqname){
	FILE *fd;
#else
int maildiropen(char *maildir, char **uniqname){
	int fd;
#endif
	char *path=NULL;

	if ((maildirfind(maildir)) == -1) goto errexit;

	maildirgname(uniqname);
	if ((cat(&path, maildirpath, "/tmp/", *uniqname, NULL))) goto errexit;
	logmsg(L_INFO, F_MAILDIR, "spooling to ", path, NULL);
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
	if ((fd=fopen(path, "w+")) == NULL) {
#else
	if ((fd=open(path, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0644)) == -1) {
#endif
		logmsg(L_ERROR, F_MAILDIR, "open ", path, " for writing failed: ", strerror(errno), NULL);
		free(path);
		goto errexit;
	}
	free(path);
	return fd;
 errexit: // with supporting windows-crap / stdio that's easier than many ifdefs
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
	return (FILE*)NULL;
#else
	return -1;
#endif
}

#if (defined(__WIN32__)) || (defined _BROKEN_IO)
int maildirclose(char *maildir, char **uniqname, FILE *fd){
#else
int maildirclose(char *maildir, char **uniqname, int fd){
#endif
	char *oldpath=NULL, *newpath=NULL;
	int status=0;
	if ((maildirfind(maildir)) == -1) {
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
		fflush(fd);
		fclose(fd);
#else
		close(fd);
#endif
		return -1;
	}

	cat(&oldpath, maildirpath, "/tmp/", *uniqname, NULL);
	cat(&newpath, maildirpath, "/new/", *uniqname, NULL);
	//cat(&newpath, maildirpath, "/cur/", *uniqname, ":2,", NULL);

#if (defined(__WIN32__)) || (defined _BROKEN_IO)
	if (!(status=fclose(fd))){
#else
	if (!(status=close(fd))){
#endif
#ifdef __WIN32__
		status=MoveFile(oldpath, newpath);
#else
		status=link(oldpath, newpath);
		unlink(oldpath);
#endif
	} else logmsg(L_ERROR, F_GENERAL, "Closing mail failed: ", strerror(errno), NULL);

	free(oldpath);
	free(newpath);
	return status;
}

int maildir_init(char *maildir, char *subdir, int harddelete){
	// maybe add a flag to recurse into subdirs
	DIR *dirptr;
	struct dirent *tmpdirent;
	char *mymaildir=NULL;
	maildirent tmpmaildirent;
	struct stat maildirstat;

	maildir_harddelete = harddelete;
	memset(&maildirstat, 0, sizeof(struct stat));
	memset(&tmpmaildirent, 0, sizeof(maildirent));
	
	if (maildirfind(maildir)){
		logmsg(L_ERROR, F_MAILDIR, "unable to find maildir", NULL);
		return -1;
	}

	if (subdir != NULL) cat(&mymaildir, maildirpath, "/", subdir, "/new", NULL);
	else cat(&mymaildir, maildirpath, "/new", NULL);

	if ((dirptr=opendir(mymaildir))==NULL){
		logmsg(L_ERROR, F_MAILDIR, "unable to open maildir ", mymaildir, ": ", strerror(errno), NULL);
		return -1;
	}

	for (tmpdirent=readdir(dirptr); tmpdirent!=NULL; tmpdirent=readdir(dirptr)){
		if (!strncmp(tmpdirent->d_name, ".", 1)) continue;
		if (!strncmp(tmpdirent->d_name, "..", 2)) continue;
		if (stat(cati(mymaildir, "/", tmpdirent->d_name, NULL), &maildirstat)==-1){
			logmsg(L_ERROR, F_MAILDIR, "stat() for file in maildir failed ", NULL);
			return -1;
		}
		strncpy(tmpmaildirent.name, tmpdirent->d_name, AM_MAXPATH);
		tmpmaildirent.size = maildirstat.st_size;
		maildir_sappend(&tmpmaildirent);
		memset(&tmpmaildirent, 0, sizeof(maildirent));
	}
	return 0; //FIXME
}

static int maildir_sappend(maildirent *maildir_add){
	maildirent *p;

	logmsg(L_DEBUG, F_MAILDIR, "adding ", maildir_add->name, " to maildir structure",  NULL);
	if (maildir_storage == NULL){
		if ((maildir_storage = malloc(sizeof(maildirent))) == NULL) {
			logmsg(L_ERROR, F_MAILDIR, "unable to malloc() memory for first maildir element", NULL);
			return -1;
		}
		maildir_cnt++;
		memcpy(maildir_storage, maildir_add, sizeof(maildirent));
		maildir_storage->next=NULL;
	} else {
		p=maildir_storage;
		while (p->next != NULL) p=p->next;

		if ((p->next=malloc(sizeof(maildirent))) == NULL) {
			logmsg(L_ERROR, F_MAILDIR, "unable to malloc() memory for new maildir element", NULL);
			return -1;
		}
		memcpy(p->next, maildir_add, sizeof(maildirent));
		maildir_cnt++;
	}
	return 0;
}
