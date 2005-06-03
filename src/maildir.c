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

#include "cat.h"
#include "aardlog.h"
#include "maildir.h"
#include "fs.h"

static int deliveries=0;
static maildir *maildir_storage;
static int maildir_cnt=0;
static int maildir_harddelete=0; // shall we delete the mails, or just mark them deleted and remove them on cleanup?

static int maildir_sappend(maildir *maildir_add);

int maildirgname(char **uniqname){
	char tmpbuf[512];
	char myhost[NI_MAXHOST];
	time_t mytime=time(NULL);
	pid_t mypid=getpid();

	if (gethostname(myhost, NI_MAXHOST)==-1){
		logmsg(L_WARNING, F_GENERAL, "unable to get hostname, setting to localhost.localdomain", NULL);
		strcpy(myhost, "localhost.localdomain");
	}

	deliveries++;
	sprintf(tmpbuf, "%li.P%iQ%i", (unsigned long)mytime, (int) mypid, deliveries);
#ifdef _POSIX_SOURCE

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
		if (getenv("HOME")==NULL){
			logmsg(L_ERROR, F_GENERAL, "$MAILDIR not set, $HOME not found", NULL);
			return -1;
		} else {
			if (cat(&maildirpath, getenv("HOME"), "/Maildir", NULL)) return -1;
			else if (!td(maildirpath)) return 0;
		}
	} else {
		maildirpath=strdup(getenv("MAILDIR"));
		if (!td(maildirpath)) return 0;
	}

	return -1; // if we got that far we did not find a usable maildir
}

// opens a file in maildir/ 
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
	if ((cat(&path, maildirpath, "/new/", *uniqname, NULL))) goto errexit;
	logmsg(L_INFO, F_GENERAL, "spooling to ", path, NULL);
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
	if ((fd=fopen(path, "w+")) == NULL) {
#else
	if ((fd=open(path, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0644)) == -1) {
#endif
		logmsg(L_ERROR, F_GENERAL, "open ", path, " for writing failed: ", strerror(errno), NULL);
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

	cat(&oldpath, maildirpath, "/new/", *uniqname, NULL);
	cat(&newpath, maildirpath, "/cur/", *uniqname, ":2,", NULL);

#if (defined(__WIN32__)) || (defined _BROKEN_IO)
	if (!(status=fclose(fd))){
#else
	if (!(status==close(fd))){
#endif
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

int maildir_init(char *maildir, char *subdir, int harddelete){
	(void) harddelete;
	// maybe add a flag to recurse into subdirs
	DIR *dirptr;
	struct dirent *tmpdirent;
	char *mymaildir=NULL;

	if (maildirfind(maildir)){
		logmsg(L_ERROR, F_GENERAL, "unable to find maildir", NULL);
		return -1;
	}

	if (subdir != NULL) cat(&mymaildir, maildirpath, "/", subdir, "/cur", NULL);
	else cat(&mymaildir, maildirpath, "/cur", NULL);

	if ((dirptr=opendir(mymaildir))==NULL){
		logmsg(L_ERROR, F_GENERAL, "unable to open maildir ", mymaildir, ": ", strerror(errno), NULL);
		return -1;
	}

	for (tmpdirent=readdir(dirptr); tmpdirent!=NULL; tmpdirent=readdir(dirptr)){
		if (!strncmp(tmpdirent->d_name, ".", 1)) continue;
		if (!strncmp(tmpdirent->d_name, "..", 2)) continue;
		logmsg(L_ERROR, F_GENERAL, "adding ", tmpdirent->d_name, " to struct", NULL);
	}
	return 0; //FIXME
}

static int maildir_sappend(maildir *maildir_add){
	maildir *p;

	if (maildir_storage == NULL){
		if ((maildir_storage = malloc(sizeof(maildir))) == NULL) {
			logmsg(L_ERROR, F_GENERAL, "unable to malloc() memory for first maildir element", NULL);
			return -1;
		}
		maildir_cnt++;
		memcpy(maildir_storage, maildir_add, sizeof(maildir));
		maildir_storage->next=NULL;
	} else {
		p=maildir_storage;
		while (p->next != NULL) p=p->next;

		if ((p->next=malloc(sizeof(maildir))) == NULL) {
			logmsg(L_ERROR, F_GENERAL, "unable to malloc() memory for new maildir element", NULL);
			return -1;
		}
		memcpy(p->next, maildir_add, sizeof(maildir));
		maildir_cnt++;
	}
	return 0;
}
