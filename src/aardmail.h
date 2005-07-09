#ifndef _AARDMAIL_H
#define _AARDMAIL_H

#include "types.h"
#ifdef __dietlibc__
#include <write12.h>
#else
#include <string.h>
#ifdef __WIN32__
#ifdef _GNUC_
#include <getopt.h>
#else
#include "getopt.h"
#endif
#include <io.h>
#endif

#define AM_VERSION "(aardmail 0.1-pre4); http://bwachter.lart.info/projects/aardmail/"

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
void kirahvi();
#endif
