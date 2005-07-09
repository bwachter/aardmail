#ifndef _ADDRLIST_H
#define _ADDRLIST_H

#ifdef __WIN32__
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif

#include "aardmail.h"

typedef struct _addrlist addrlist;

struct _addrlist {
	char address[AM_MAXUSER];
	int accepted;
	addrlist *next;
};

int addrlist_append(addrlist **addrlist_storage, char *address);
int addrlist_delete(addrlist **addrlist_storage, char *address);
#endif
