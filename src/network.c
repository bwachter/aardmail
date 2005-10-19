#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
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

#ifdef __WIN32__
#define EAI_ADDRFAMILY   -9    /* Address family for NAME not supported.  */
#define EAI_SYSTEM       -11   /* System error returned in `errno'.  */
#endif

#if (defined(__WIN32__)) || (defined(_BROKEN_IO))
static struct addrinfo *
dup_addrinfo (struct addrinfo *info, void *addr, size_t addrlen)
{
  struct addrinfo *ret;

  ret = malloc (sizeof (struct addrinfo));
  if (ret == NULL)
    return NULL;
  memcpy (ret, info, sizeof (struct addrinfo));
  ret->ai_addr = malloc (addrlen);
  if (ret->ai_addr == NULL)
    {
      free (ret);
      return NULL;
    }
  memcpy (ret->ai_addr, addr, addrlen);
  ret->ai_addrlen = addrlen;
  return ret;
}
#endif

#if (defined __WIN32__) || (defined _BROKEN_IO)
void
netfreeaddrinfo (struct addrinfo *ai)
{
  struct addrinfo *next;

  while (ai != NULL)
    {
      next = ai->ai_next;
      if (ai->ai_canonname != NULL)
        free (ai->ai_canonname);
      if (ai->ai_addr != NULL)
        free (ai->ai_addr);
      free (ai);
      ai = next;
    }
}
#endif

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
	int i;

	logmsg(L_INFO, F_NET, "> ", buf, NULL);

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
#ifdef __GNUC__
		(void) flags;
		(void) salen;
#endif
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
  struct hostent *hp;
  struct servent *servent;
  const char *socktype;
  int port;
  struct addrinfo hint, result;
  struct addrinfo *ai, *sai, *eai;
  char **addrs;
	err=0;

  memset (&result, 0, sizeof result);

  /* default for hints */
  if (hints == NULL)
    {
      memset (&hint, 0, sizeof hint);
      hint.ai_family = PF_UNSPEC;
      hints = &hint;
    }

  /* servname must not be NULL in this implementation */
  if (service == NULL)
    return EAI_NONAME;

  /* check for tcp or udp sockets only */
  if (hints->ai_socktype == SOCK_STREAM)
    socktype = "tcp";
  else if (hints->ai_socktype == SOCK_DGRAM)
    socktype = "udp";
  else
    return EAI_SERVICE;
  result.ai_socktype = hints->ai_socktype;

  /* Note: maintain port in host byte order to make debugging easier */
  if (isdigit (*service))
    port = strtol (service, NULL, 10);
  else if ((servent = getservbyname (service, socktype)) != NULL)
    port = ntohs (servent->s_port);
  else
    return EAI_NONAME;

  /* if nodename == NULL refer to the local host for a client or any
     for a server */
  if (node == NULL)
    {
      struct sockaddr_in sin;

      /* check protocol family is PF_UNSPEC or PF_INET - could try harder
         for IPv6 but that's more code than I'm prepared to write */
      if (hints->ai_family == PF_UNSPEC || hints->ai_family == PF_INET)
	result.ai_family = AF_INET;
      else
	return EAI_FAMILY;

      sin.sin_family = result.ai_family;
      sin.sin_port = htons (port);
      if (hints->ai_flags & AI_PASSIVE)
        sin.sin_addr.s_addr = htonl (INADDR_ANY);
      else
        sin.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
      /* Duplicate result and addr and return */
      *res = dup_addrinfo (&result, &sin, sizeof sin);
      return (*res == NULL) ? EAI_MEMORY : 0;
    }

  /* If AI_NUMERIC is specified, use inet_addr to translate numbers and
     dots notation. */
  if (hints->ai_flags & AI_NUMERICHOST)
    {
      struct sockaddr_in sin;

      /* check protocol family is PF_UNSPEC or PF_INET */
      if (hints->ai_family == PF_UNSPEC || hints->ai_family == PF_INET)
	result.ai_family = AF_INET;
      else
	return EAI_FAMILY;

      sin.sin_family = result.ai_family;
      sin.sin_port = htons (port);
      sin.sin_addr.s_addr = inet_addr (node);
      /* Duplicate result and addr and return */
      *res = dup_addrinfo (&result, &sin, sizeof sin);
      return (*res == NULL) ? EAI_MEMORY : 0;
    }

  hp = gethostbyname (node);
	// fixme, translate error codes
  if (hp == NULL) return h_errno;

  /* Check that the address family is acceptable.
   */
  switch (hp->h_addrtype)
    {
    case AF_INET:
      if (!(hints->ai_family == PF_UNSPEC || hints->ai_family == PF_INET))
				return EAI_FAMILY;
      break;
    case AF_INET6:
      if (!(hints->ai_family == PF_UNSPEC || hints->ai_family == PF_INET6))
				return EAI_FAMILY;
      break;
    default:
      return EAI_FAMILY;
    }

  /* For each element pointed to by hp, create an element in the
     result linked list. */
  sai = eai = NULL;
  for (addrs = hp->h_addr_list; *addrs != NULL; addrs++)
    {
      struct sockaddr sa;
      size_t addrlen;

      if (hp->h_length < 1)
        continue;
      sa.sa_family = hp->h_addrtype;
      switch (hp->h_addrtype)
        {
        case AF_INET:
	  ((struct sockaddr_in *) &sa)->sin_port = htons (port);
	  memcpy (&((struct sockaddr_in *) &sa)->sin_addr,
	          *addrs, hp->h_length);
          addrlen = sizeof (struct sockaddr_in);
          break;
        case AF_INET6:
# if SIN6_LEN
	  ((struct sockaddr_in6 *) &sa)->sin6_len = hp->h_length;
# endif
	  ((struct sockaddr_in6 *) &sa)->sin6_port = htons (port);
	  memcpy (&((struct sockaddr_in6 *) &sa)->sin6_addr,
	  	  *addrs, hp->h_length);
          addrlen = sizeof (struct sockaddr_in6);
          break;
        default:
          continue;
        }

      result.ai_family = hp->h_addrtype;
      ai = dup_addrinfo (&result, &sa, addrlen);
      if (ai == NULL)
        {
          netfreeaddrinfo (sai);
          return EAI_MEMORY;
        }
      if (sai == NULL)
	sai = ai;
      else
	eai->ai_next = ai;
      eai = ai;
    }

  if (sai == NULL)
    {
      return EAI_NODATA;
    }
  
  if (hints->ai_flags & AI_CANONNAME) 
    {
      sai->ai_canonname = malloc (strlen (hp->h_name) + 1);
      if (sai->ai_canonname == NULL)
        {
          netfreeaddrinfo (sai);
          return EAI_MEMORY;
        }
      strcpy (sai->ai_canonname, hp->h_name);
    }

  *res = sai;
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
#ifdef __WIN32__
	WSADATA wsaData;
#endif

	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;

#ifdef __WIN32__
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
