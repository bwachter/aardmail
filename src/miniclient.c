/**
 * @file miniclient.c
 * @author Bernd Wachter <bwachter@lart.info>
 * @date 2005-2011
 */

#include <stdlib.h>
#include <stdio.h>
#ifdef __WIN32__
#include <windows.h>
#include <winbase.h>
#ifdef _GNUC_
#include <getopt.h>
#else
#include <ibaard_getopt.h>
#endif
#else
#include <getopt.h>
#include <unistd.h>
#include <sys/poll.h>
#endif

#include <ibaard_network.h>
#include <ibaard_log.h>
#include <ibaard_cat.h>
#include <ibaard_authinfo.h>

#include "aardmail.h"

/** Print usage information and exit()
 *
 */
static void miniclient_usage(char *program){
  char *tmpstring=NULL;
  if (!cat(&tmpstring, "Usage: ", program,
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
           " [-c option] -h hostname -s service [-t] [-v level]\n",
#else
           " -h hostname -s service [-v level]\n",
#endif
           "\t-a:\tbind to given address (hostnames will be resolved)\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
           "\t-c:\tcrypto options. Options may be: 0 (off), 1 (tls, like -t),\n",
           "\t\tand 2 (tls, fallback to plain on error)\n",
#endif
           "\t-h:\tspecify the hostname to connect to\n",
           "\t-s:\tthe service to connect to. Must be resolvable if non-numeric.\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
           "\t-t:\tuse tls. If tls is not possible the program will exit (like -c 1)\n",
#endif
           "\t-v:\tset the loglevel, valid values are 0 (no logging), 1 (deadly),\n",
           "\t\t2 (errors, default), 3 (warnings) and 4 (info, very much)\n",
           "\n[ ", AM_VERSION, " ]\n",
           NULL)) {
    __write2(tmpstring);
    free(tmpstring);
  }
  exit(0);
}

/** Main entry point
 *
 * @param argc Argument count
 * @param argv Argument array
 * @return -1 on error, 0 on success
 */
int main(int argc, char** argv){
  char buf[1024];

#ifdef __WIN32__
  HANDLE myhandles[2];
  int res;
  WSAEVENT event = WSACreateEvent();
#else
  struct pollfd pfd[2];
#endif
  int i, sd;
  authinfo defaultauth;
  char *bindname=NULL;

  memset(&defaultauth, 0, sizeof(authinfo));

  while((i=getopt(argc, argv,
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
                  "a:c:f:h:s:tv:"
#else
                  "a:h:s:v:"
#endif
           )) != EOF){
    switch(i){
      case 'a':
        bindname = strdup(optarg);
        break;
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
      case 'c':
        switch(atoi(optarg)){
          case 0: am_sslconf=0; break;
          case 1: am_sslconf=AM_SSL_USETLS; break;
          case 2: am_sslconf=AM_SSL_USETLS & AM_SSL_ALLOWPLAIN; break;
        }
        break;
      case 'f':
        strncpy(am_sslkey, optarg, 1024);
        break;
#endif
      case 'h':
        strncpy(defaultauth.machine, optarg, NI_MAXHOST);
        break;
      case 's':
        strncpy(defaultauth.port, optarg, NI_MAXSERV);
        break;
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
      case 't':
        am_sslconf = AM_SSL_USETLS;
        break;
#endif
      case 'v':
        loglevel(atoi(optarg));
        break;
    } // switch
  } // while

  if (!strcmp(defaultauth.machine,""))
    miniclient_usage(argv[0]);
  if (!strcmp(defaultauth.port, ""))
    miniclient_usage(argv[0]);

  if ((sd=netconnect2(defaultauth.machine, defaultauth.port, bindname))==-1)
    return -1;

#ifdef __WIN32__
#ifdef __GNUC__
#warning miniclient is not fully working due to the braindead Windows API
#endif

  i=netreadline(sd, buf);
  __write2(buf);

  myhandles[0] = GetStdHandle(STD_INPUT_HANDLE);
  if (WSAEventSelect(sd, event, FD_READ)!=0){
    logmsg(L_ERROR, F_NET, "WSAEventSelect() failed", NULL);
    return -1;
  }
  myhandles[1] = (HANDLE)event;

  while (strcmp(buf, "exit\n")){
    res = WaitForMultipleObjects(2, myhandles, FALSE, INFINITE);
    if (res == WAIT_OBJECT_0) {
      if ((i=read(0,buf,1024))==-1) return -1;
      buf[i]='\0';
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
      if ((!strcasecmp(buf, "STLS\n")) || (!strcasecmp(buf, "STARTTLS\n")))
        am_sslconf=AM_SSL_STARTTLS;
#endif
      netwriteline(sd, buf);
    } else if (res == WAIT_OBJECT_0+1) {
      // for seme reason we get here always when there's no input from stdin.
      if ((i=netreadline(sd,buf))==-1) { }
      __write1(buf);
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
      if (am_sslconf==AM_SSL_STARTTLS) {
        if ((!strncmp(buf, "+OK", 3)) || (!strncmp(buf, "220", 3))) {
          am_sslconf = AM_SSL_USETLS;
          if (netsslstart(sd)){
            logmsg(L_ERROR, F_SSL, "unable to start ssl negotiation", NULL);
            close(sd);
            return -1;
          }
        } else am_sslconf=0;
      }
#endif
    }
  }
#else
  // the UNIX part
  pfd[0].fd=0;
  pfd[0].events=POLLRDNORM | POLLIN;
  pfd[1].fd=sd;
  pfd[1].events=POLLRDNORM | POLLIN;

  i=netreadline(sd, buf);
  __write2(buf);

  while (strcmp(buf, "exit\n")){
    poll(pfd, 2, -1);
    // poll on STDIN
    if (pfd[0].revents & (POLLRDNORM | POLLIN)){
      if ((i=read(pfd[0].fd,buf,1024))==-1) return -1;
      buf[i]='\0';
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
      if ((!strcasecmp(buf, "STLS\n")) || (!strcasecmp(buf, "STARTTLS\n")))
        am_sslconf=AM_SSL_STARTTLS;
#endif
      netwriteline(pfd[1].fd, buf);
    }
    // poll on network descriptor
    if (pfd[1].revents & (POLLRDNORM | POLLIN)){
      if ((i=netreadline(pfd[1].fd,buf))==-1) return -1;
      if (i==0) break; // peer closed connection
      __write1(buf);
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
      if (am_sslconf==AM_SSL_STARTTLS) {
        if ((!strncmp(buf, "+OK", 3)) || (!strncmp(buf, "220", 3))) {
          am_sslconf = AM_SSL_USETLS;
          if (netsslstart(pfd[1].fd)){
            logmsg(L_ERROR, F_SSL, "unable to start ssl negotiation", NULL);
            close(pfd[1].fd);
            return -1;
          }
        } else am_sslconf=0;
      }
#endif
    }
  }
#endif

  return 0;
}
