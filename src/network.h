#ifndef _NETWORK_H
#define _NETWORK_H
#ifdef __WIN32__
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#endif

#include "aardmail.h"

#define MAXNETBUF 1024
#define AM_SSL_ALLOWPLAIN 1
#define AM_SSL_USETLS 2
#define AM_SSL_STARTTLS 4

#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
int am_sslconf;
char am_sslkey[1024];
#endif

#ifdef HAVE_SSL
#include <openssl/ssl.h>
SSL *ssl;
#endif

#ifdef HAVE_MATRIXSSL
#include <matrixSsl.h>
typedef struct {
	ssl_t *ssl;
	sslBuf_t inbuf;
	sslBuf_t insock;
	sslBuf_t outsock;
	int outBufferCount;
	int fd;
} sslConn_t;
sslConn_t *ssl;
#endif

int netconnect(char *hostname, char *servicename);
int netread(int sd, char *buf);
int netreadline(int sd, char *buf);
//int netwrite(int sd, char *buf);
int netwriteline(int sd, char *buf);
int netnameinfo(const struct sockaddr *sa, socklen_t salen, 
						char *hostname, size_t hostlen, 
						char *servname, size_t servlen, int flags);

#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
int netsslstart(int sd);
#endif
#ifdef HAVE_SSL
int netsslread(SSL *ssl_handle, char *buf, int len);
int netsslwrite(SSL *ssl_handle, char *buf, int len);
#endif
#ifdef HAVE_MATRIXSSL
int netsslread(sslConn_t *ssl_handle, char *buf, int len);
int netsslwrite(sslConn_t *ssl_handle, char *buf, int len);
#endif
#endif
