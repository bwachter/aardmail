#ifndef _MAILDIR_H
#define _MAILDIR_H

#include <sys/stat.h>

#if (defined(__WIN32__)) || (defined _BROKEN_IO)
#include <stdio.h>
#endif

#include "aardmail.h"

typedef struct _maildirent maildirent;

struct _maildirent {
	char name[AM_MAXPATH];
	off_t size;
	int deleted;
	maildirent *next;
};

char *maildirpath;
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
FILE *maildiropen(char *maildir, char **uniqname);
int maildirclose(char *maildir, char **uniqname, FILE* fd);
#else
int maildiropen(char *maildir, char **uniqname);
int maildirclose(char *maildir, char **uniqname, int fd);
#endif
int maildir_init(char *maildir, char *subdir, int harddelete);
int maildirfind(char *maildir);

#endif
