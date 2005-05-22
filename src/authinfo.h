#ifndef _AUTHINFO_H
#define _AUTHINFO_H

#ifdef __WIN32__
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif

typedef struct _authinfo authinfo;
typedef struct _authinfo_key authinfo_key;

struct _authinfo {
	char machine[NI_MAXHOST];
	char service[NI_MAXSERV];
	char *login;
	char *password;
	authinfo *next;
	authinfo *prev;
};

struct _authinfo_key {
	char *name;
	int hasargs;
};

int authinfo_init();

#endif
