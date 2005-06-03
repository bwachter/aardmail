#ifndef _MAILDIR_H
#define _MAILDIR_H
#include <sys/stat.h>

typedef struct _maildir maildir;

struct _maildir {
	struct stat *stat;
	int deleted;
	maildir *next;
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

#endif
