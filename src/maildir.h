#ifndef _MAILDIR_H
#define _MAILDIR_H

char *maildirpath;
int maildiropen(char *maildir, char **uniqname);
int maildirclose(char *maildir, char **uniqname, int fd);
#endif
