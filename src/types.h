#ifndef _AM_TYPES_H
#define _AM_TYPES_H

#ifdef __WIN32__
#include <io.h>
#include <windows.h>
#include <winbase.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdint.h>
#endif

#ifdef __WIN32__
#ifndef strcasecmp
#define strcasecmp(a,b) stricmp(a,b)
#endif 
#ifndef strncasecmp
#define strncasecmp(a,b,c) strnicmp(a,b,c)
#endif
#endif

#ifdef __BORLANDC__
#define pclose(a) _pclose(a)
#define popen(a,b) _popen(a,b)
#endif

#define AM_MAXUSER 1025
#define AM_MAXPASS 1025
#define AM_MAXPATH 1025

#ifdef __BORLANDC__
struct addrinfo {
	int     ai_flags;
	int     ai_family;
	int     ai_socktype;
	int     ai_protocol;
	size_t  ai_addrlen;
	struct sockaddr *ai_addr;
	char   *ai_canonname;
	struct addrinfo *ai_next;
};

typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef uint32_t socklen_t;
typedef int pid_t;
#endif

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif
#ifndef NI_MAXSERV
#define NI_MAXSERV 32
#endif
#ifndef NI_NUMERICSERV
#define NI_NUMERICSERV 8
#endif
#ifndef NU_NUMERICHOST
#define NI_NUMERICHOST 2
#endif

#ifndef EAI_FAMILY
#define EAI_FAMILY -1
#endif
#ifndef EAI_NONAME
#define EAI_NONAME -4
#endif
#ifndef EAI_SERVICE
#define EAI_SERVICE -5
#endif
#ifndef EAI_NODATA
#define EAI_NODATA -7
#endif
#ifndef EAI_MEMORY
#define EAI_MEMORY -8
#endif
#ifndef EAI_FAIL
#define EAI_FAIL -9
#endif
#ifndef EAI_AGAIN
#define EAI_AGAIN -10
#endif

// socket stuff
#ifndef AI_NUMERICHOST
#define AI_NUMERICHOST 1
#endif
#ifndef AI_CANONNAME
#define AI_CANONNAME 2
#endif
#ifndef AI_PASSIVE
#define AI_PASSIVE 4
#endif
#endif
