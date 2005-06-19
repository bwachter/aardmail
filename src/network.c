#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#ifdef __WIN32__
#include <windows.h>
#include <winbase.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "aardmail.h"
#include "aardlog.h"
#include "network.h"
#include "cat.h"

int netread(int sd, char *buf){
	int i;
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
	if (am_sslconf & AM_SSL_USETLS){
		i=netsslread(ssl, buf, MAXNETBUF);
	} else 
#endif
		i=recv(sd, buf, MAXNETBUF, 0);
	buf[i]='\0';
	return i;
}

int netreadline(int sd, char *buf){
	int i,cnt;
	char tmpbuf[1];

	buf[0]='\0';
	for (cnt=0; cnt<MAXNETBUF-2; cnt++){
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
		if (am_sslconf & AM_SSL_USETLS){
			i=netsslread(ssl, tmpbuf, 1);
		} else
#endif
			i=recv(sd, tmpbuf, 1, 0);
		if (i == -1) return -1;
		else{
			if (tmpbuf[0] == '\0'){
				// nullbyte handling will work as long as any functions using
				// our output won't rely on strlen()
				logmsg(L_WARNING, F_NET, "nullbyte detected, mail might be corrupted", NULL);
				logmsg(L_INFO, F_NET, "< ", buf, NULL);
				return cnt;
			} else {
				strncat(buf, tmpbuf, 1);
				if (buf[cnt] == '\n' && buf[cnt-1]=='\r'){
					logmsg(L_INFO, F_NET, "< ", buf, NULL);
					return cnt; 
				}
			}
		}
	}
	logmsg(L_INFO, F_NET, "< ", buf, NULL);
	return MAXNETBUF-3;
}

int netwriteline(int sd, char *buf){
	logmsg(L_INFO, F_NET, "> ", buf, NULL);
	int i;
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
	if (am_sslconf & AM_SSL_USETLS){
		i=netsslwrite(ssl, buf, strlen(buf));
	} else
#endif
		i=send(sd, buf, strlen(buf), 0);
	return i;
}

void netlogportservice(const struct sockaddr *sa, socklen_t salen, char *msg){
	char host[NI_MAXHOST];
	char serv[NI_MAXSERV];
	char *tmpstring;

	netnameinfo(sa, salen, host, sizeof(host),serv, sizeof(serv),
							NI_NUMERICHOST | NI_NUMERICSERV);
	cat(&tmpstring, msg, host, ":", serv, "\n", NULL);
	__write2(tmpstring);
	free(tmpstring);
}

int netnameinfo(const struct sockaddr *sa, socklen_t salen, 
						char *hostname, size_t hostlen, 
						char *servname, size_t servlen, int flags){
#ifdef __WIN32__
	HINSTANCE _hInstance = LoadLibrary( "ws2_32" );
	int (WSAAPI *pfn_getnameinfo) (const struct sockaddr*, socklen_t salen,
																 char *hostname, size_t hostlen,
																 char *servname, size_t servlen, int flags);

	pfn_getnameinfo =	GetProcAddress( _hInstance, "getnameinfo" );

	if (pfn_getnameinfo){
		return (pfn_getnameinfo(sa, salen, hostname, hostlen, servname, servlen, flags));
	} else {
#endif
#if (defined( __WIN32__)) || (defined(_BROKEN_IO))
		(void) flags;
		(void) salen;
		char *tmp;
		if ((tmp = malloc((NI_MAXHOST+1)*sizeof(char))) == NULL) {
			__write2("malloc() failed\n");
			return -1;
		}

		if (servname != NULL) {
			uint16_t service_int;
			service_int = ntohs(((struct sockaddr_in*)sa)->sin_port);
			snprintf(tmp, NI_MAXHOST, "%i", service_int);
			strncpy(servname, tmp, servlen);
		}

		free(tmp);

		if (hostname != NULL) {
			if ((tmp = inet_ntoa(((struct sockaddr_in*)sa)->sin_addr)) == NULL){
				__write2("converting ip failed\n");
			}
			strncpy(hostname, tmp, hostlen);
		}
		return 0;
#ifdef __WIN32__
	}
#endif
#else
	return (getnameinfo(sa, salen, hostname, hostlen, servname, servlen, flags));
#endif
}

int netaddrinfo(const char *node, const char *service, 
								const struct addrinfo *hints, struct addrinfo **res){
	int err;
#ifdef __WIN32__
	HINSTANCE _hInstance = LoadLibrary( "ws2_32" );
	int (WSAAPI *pfn_getaddrinfo) (const char*, const char*, const struct addrinfo*, struct addrinfo **);

	pfn_getaddrinfo =	GetProcAddress( _hInstance, "getaddrinfo" );

	if (pfn_getaddrinfo){
		return (err=pfn_getaddrinfo(node, service, hints, res));
	} else {
#endif
#if (defined(__WIN32__)) || (defined(_BROKEN_IO))
#warning my getaddrinfo-emulation is currently broken. Your build should be working under Windows XP -- but nowhere else.
		struct addrinfo **aic;
		struct addrinfo ai;
		struct hostent *hent;
		struct servent *sent;
		struct sockaddr_in saddr;
		memset(&saddr, 0, sizeof saddr);
		//memset(&ai, 0, sizeof(struct addrinfo));
		aic=res; 
		*res=0;
		
		if ((hent = gethostbyname(node)) == NULL) {
			__write2("gethostbyneme() failed\n");
			return -1;
		}

		sent=malloc(sizeof(struct sockaddr_in));
		if (!(err=strtol(service, (char **)NULL, 10))) {
			if ((sent = getservbyname(service, NULL)) == NULL) {
				__write2("getservbyname() failed\n");
				return -1;
			}
			saddr.sin_port = sent->s_port;
		}else
			saddr.sin_port = htons(err);

		memcpy(&saddr.sin_addr, hent->h_addr, sizeof(saddr.sin_addr));
		saddr.sin_family = hent->h_addrtype;
		//ai->ai_flags = 
		//ai.ai_family = hent->h_addrtype;
		ai.ai_family = AF_INET;
		ai.ai_socktype = SOCK_STREAM;
		ai.ai_protocol = IPPROTO_TCP;
		ai.ai_addrlen = sizeof(saddr);
		ai.ai_addr = (struct sockaddr *) &saddr;
		//ai.ai_canonname = "foo";
		ai.ai_next = (struct addrinfo*)NULL;
		//*aic=&(ai);
		*aic=malloc(sizeof(struct addrinfo));
		if (!*aic) *aic=&ai; 
		else exit(0);
		//else (*aic)->ai_next=&ai;
		//memcpy(&*res, &ai, sizeof(struct addrinfo));
		//*res=&ai;
		//netlogportservice(ai.ai_addr, ai.ai_addrlen, "Connection: ");
		//exit (0);
		return 0;
#ifdef __WIN32__
	}
#endif
#else
	(void)err;
	return (getaddrinfo(node, service, hints, res));
#endif
}

static int netsocket(struct addrinfo *ai){
	int sd;
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
	int err;
#endif

	if (debuglevel > 0)
		netlogportservice(ai->ai_addr, ai->ai_addrlen, "Trying to connect to: ");

	if ((sd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0){
		logmsg(L_ERROR, F_NET, "socket() failed: ", strerror(errno), NULL);
		return -1;
	}

	if (connect(sd, ai->ai_addr, ai->ai_addrlen) < 0){
		logmsg(L_ERROR, F_NET, "connect() failed: ", strerror(errno), NULL);
		return -1;
	}

#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
	if (am_sslconf & AM_SSL_USETLS){
		if ((err=netsslstart(sd)) && (am_sslconf & AM_SSL_ALLOWPLAIN)){
			logmsg(L_WARNING, F_NET, "no ssl available, continuing from start", NULL);
			close(sd);
			return netsocket(ai);
		} else if (err && !(am_sslconf & AM_SSL_ALLOWPLAIN)){
			logmsg(L_DEADLY, F_NET, "unable to open ssl connection, plaintext fallback disabled.", NULL);
		}
	}
#endif
	return sd;
}

// opens a network connection and returns a socket descriptor
int netconnect(char *hostname, char *servicename){
	struct addrinfo *res;
	struct addrinfo hints;
	int sd, err;

	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;

#ifdef __WIN32__
	WSADATA wsaData;
	WSAStartup( 0x0202, &wsaData );
#endif
	if ((err=netaddrinfo(hostname, servicename, &hints, &res))){
		logmsg(L_ERROR, F_NET, "unable to resolve host", NULL);
		return -1;
	} else {
		while (res){
			if ((sd=netsocket(&*res)) > 0)
				return sd;
			if(res->ai_next==NULL)
				logmsg(L_INFO, F_NET, "res->ai_next is NULL", NULL);
			else 
				logmsg(L_INFO, F_NET, "trying next element", NULL);
			res=res->ai_next;
		}
	}
	return -1;
}
