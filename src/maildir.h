#ifndef _MAILDIR_H
#define _MAILDIR_H

char *maildirpath;
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
FILE *maildiropen(char *maildir, char **uniqname);
int maildirclose(char *maildir, char **uniqname, FILE* fd);
#else
int maildiropen(char *maildir, char **uniqname);
int maildirclose(char *maildir, char **uniqname, int fd);
#endif

#endif
