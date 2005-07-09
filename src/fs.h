#ifndef _FS_H
#define _FS_H

#if (defined __WIN32__) || (defined _BROKEN_IO)
#include <stdio.h>
#endif

int tf(char *name);
int td(char *name);
int openreadclose(char *fn, char **buf, unsigned long *len);
#if (defined __WIN32__) || (defined _BROKEN_IO)
int filewrite(FILE *fd, char *buf, int len);
#else
int filewrite(int fd, char *buf, int len);
#endif

#endif
