#ifndef _AARDMAIL_H
#define _AARDMAIL_H

#ifdef __dietlibc__
#include <write12.h>
#else
#include <string.h>
#include <unistd.h>
static inline int __write1(const char *s){
	write(1, s, strlen(s));
	return 0;
}

static inline int __write2(const char *s){
	write(2, s, strlen(s));
	return 0;
}
#endif

int debuglevel;

#endif
