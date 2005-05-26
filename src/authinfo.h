#ifndef _AUTHINFO_H
#define _AUTHINFO_H

#ifdef __WIN32__
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif

#include "aardmail.h"

typedef struct _authinfo authinfo;
typedef struct _authinfo_key authinfo_key;

struct _authinfo {
	char machine[NI_MAXHOST];
	char port[NI_MAXSERV];
	char login[AM_MAXUSER];
	char password[AM_MAXPASS];
	int defaultauth;
	int force;
	authinfo *next;
};

struct _authinfo_key {
	char *name;
	int hasargs;
};

int authinfo_init();
int authinfo_lookup(authinfo *authinfo_lookup);
int authinfo_lookup(authinfo *authinfo_keys);

#endif
