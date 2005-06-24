#ifndef _AARDMAIL_H
#define _AARDMAIL_H

#ifdef __dietlibc__
#include <write12.h>
#else
#include <string.h>
#ifdef __WIN32__
#include <io.h>
#else
#include <unistd.h>
#endif
#include "types.h"

#ifdef __BORLANDC__
static int __write1(const char *s)
#else
static inline int __write1(const char *s)
#endif
{
	write(1, s, strlen(s));
	return 0;
}

#ifdef __BORLANDC__
static int __write2(const char *s)
#else
static inline int __write2(const char *s)
#endif
{
	write(2, s, strlen(s));
	return 0;
}
#endif

int debuglevel;
int am_checkprogram(char *program);
#if (!defined(__WIN32__)) && (!defined _BROKEN_IO)
int am_pipe(char *pipeto);
#endif
void am_unimplemented();
#endif
