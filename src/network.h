#ifndef _NETWORK_H
#define _NETWORK_H
#ifdef __WIN32__
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif

#define MAXNETBUF 1024

#ifdef HAVE_SSL
#include <openssl/ssl.h>
SSL *ssl;
int use_tls;
int allow_plaintext;
#endif

int netconnect(char *hostname, char *servicename);
int netread(int sd, char *buf);
int netreadline(int sd, char *buf);
//int netwrite(int sd, char *buf);
int netwriteline(int sd, char *buf);
int netnameinfo(const struct sockaddr *sa, socklen_t salen, 
						char *hostname, size_t hostlen, 
						char *servname, size_t servlen, int flags);

int netsslstart(int sd);
#endif
