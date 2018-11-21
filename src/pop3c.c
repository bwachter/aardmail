/**
 * @file pop3c.c
 * @author Bernd Wachter <bwachter@lart.info>
 * @date 2005-2011
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#ifdef __WIN32__
#ifdef _GNUC_
#include <getopt.h>
#else
#include <ibaard_getopt.h>
#endif
#else
#include <sys/wait.h>
#include <getopt.h>
#endif

#include <ibaard_network.h>
#include <ibaard_log.h>
#include <ibaard_cat.h>
#include <ibaard_fs.h>
#include <ibaard_maildir.h>

#include "aardmail.h"
#include "aardmail-pop3c.h"

/** Print usage information and exit()
 *
 * @param program path to the program to prefix help with (usually argv[0]
 */
static void pop3c_usage(char *program);

/// Generic buffer to temporary copy strings into
static char *tmpstring=NULL;

/** Main entry point
 *
 * @param argc Argument count
 * @param argv Argument array
 * @return -1 on error, 0 on success
 */
int main(int argc, char** argv){
  int c, sd;
  authinfo defaultauth;

  memset(&defaultauth, 0, sizeof(authinfo));

  // initialize with sane values
  pop3c.keepmail = 0;
  pop3c.msgcount = 0;
  pop3c.onlyget = 0;

#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
  am_sslconf = AM_SSL_USETLS;
  netsslcacert(".aardmail");
  am_ssl_paranoid = L_DEADLY;
#endif

  while ((c=getopt(argc, argv,
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
                   "a:b:c:df:g:h:lm:n:op:r:s:tu:v:x:"
#else
                   "a:b:df:h:m:n:op:r:s:u:v:x:"
#endif
            )) != EOF){
    switch(c){
      case 'a':
        pop3c.bindname = strdup(optarg);
        break;
      case 'b':
        if (am_checkprogram(optarg)!=0) {
          logmsg(L_INFO, F_GENERAL, "not polling because program evaluated to false", NULL);
          exit(0);
        }
        break;
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
      case 'c':
        switch(atoi(optarg)){
          case 0: am_sslconf=0; break;
          case 1: am_sslconf=AM_SSL_USETLS; break;
          case 2: am_sslconf=AM_SSL_USETLS | AM_SSL_ALLOWPLAIN; am_ssl_paranoid = L_WARNING; break;
          case 3: am_sslconf=AM_SSL_STARTTLS; break;
          case 4: am_sslconf=AM_SSL_STARTTLS | AM_SSL_ALLOWPLAIN; am_ssl_paranoid = L_WARNING; break;
        }
        break;
#endif
      case 'd':
        pop3c.keepmail = 1;
        break;
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
      case 'f':
        strncpy(am_sslkey, optarg, 1024);
        break;
      case 'g':
        strncpy(am_ssl_servercerts, optarg, 1024);
        break;
#endif
      case 'h':
        strncpy(defaultauth.machine, optarg, NI_MAXHOST);
        break;
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
      case 'l':
        am_sslconf = AM_SSL_STARTTLS;
        break;
#endif
      case 'm':
        pop3c.maildir = strdup(optarg);
        break;
      case 'n':
        pop3c.onlyget = atoi(optarg);
        break;
      case 'o':
        pop3c.onlystat = 1;
        break;
      case 'p':
        logmsg(L_WARNING, F_GENERAL, "do not use -p password, it's insecure. use .authinfo", NULL);
        strncpy(defaultauth.password, optarg, AM_MAXPASS);
        break;
      case 'r':
        am_unimplemented();
        break;
      case 's':
        if (!strcmp(optarg, "kirahvi")){kirahvi(); exit(0);}
        strncpy(defaultauth.port, optarg, NI_MAXSERV);
        break;
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
      case 't':
        am_sslconf = AM_SSL_USETLS;
        break;
#endif
      case 'u':
        strncpy(defaultauth.login, optarg, AM_MAXUSER);
        break;
      case 'v':
        loglevel(atoi(optarg));
        break;
      case 'x':
        pop3c.pipeto = strdup(optarg);
        break;
    }
  }

  if (!strcmp(defaultauth.machine,""))
    pop3c_usage(argv[0]);

  if (!authinfo_init())
    if (authinfo_lookup(&defaultauth)==-1)
      logmsg(L_WARNING, F_GENERAL, "no record found in authinfo", NULL);

  if (!strcmp(defaultauth.port,""))
    strcpy(defaultauth.port, "995");

  logmsg(L_INFO, F_GENERAL, "connecting to machine ", defaultauth.machine, ", port ", defaultauth.port, NULL);
  if (strcmp(defaultauth.login, "")) logmsg(L_INFO, F_GENERAL, "using login-name: ", defaultauth.login, NULL);
  if (strcmp(defaultauth.login, "")) logmsg(L_INFO, F_GENERAL, "using password: yes", NULL);

  if ((sd=pop3c_connectauth(&defaultauth))==-1) exit(-1);
  if (pop3c_session(sd)==-1) exit(-1);
  if (pop3c_quitclose(sd)==-1) exit(-1);
  return 0;
}

static void pop3c_usage(char *program){
  if (!cat(&tmpstring, "Usage: ", program,
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
           " [-b program] [-c option] [-d] [-f certificate]\n",
           "\t\t[-g certificate] -h hostname [-l] [-m maildir] [-n number]\n",
           "\t\t[-p password] [-r number] [-s service] [-t] [-u user]\n",
           "\t\t[-v level] [-x program]\n",
#else
           " [-b program] [-d] -h hostname [-m maildir] [-p password]\n",
           "\t\t[-r number] [-s service] [-t] [-u user] [-x program]","\n",
#endif
           "\t-a:\tbind to given address (hostnames will be resolved)\n",
           "\t-b:\tonly fetch mail if program exits with zero status\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
           "\t-c:\tcrypto options. Options may be: 0 (off), 1 (tls, like -t),\n",
           "\t\t2 (tls, fallback to plain on error), 3 (starttls, no fallback)\n",
           "\t\tand 4 (starttls, fallback to plain on error). Default is 1.\n",
#endif
           "\t-d:\tdon't delete mail after retrieval (default is to delete)\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
           "\t-f:\tthe certificate file to use for authentification\n",
           "\t-g:\tthe certificate file to use for verification\n",
#endif
           "\t-h:\tspecify the hostname to connect to\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
           "\t-l:\tuse starttls, exit on error (like -c 3)\n",
#endif
           "\t-m:\tthe maildir for spooling; default (unless -x used) is ~/Maildir\n",
           "\t-n:\tonly load number mails\n",
           "\t-p:\tthe password to use. Don't use this option.\n",
           "\t-r:\treconnect after number mails (see FAQ)\n",
           "\t-s:\tthe service to connect to. Must be resolvable if non-numeric.\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
           "\t-t:\tuse tls. If tls is not possible the program will exit (like -c 1)\n",
           "\t\tThis is the default setting.\n",
#endif
           "\t-u:\tthe username to use. You usually don't need this option.\n",
           "\t-v:\tset the loglevel, valid values are 0 (no logging), 1 (deadly),\n",
           "\t\t2 (errors, default), 3 (warnings) and 4 (info, very much)\n",
           "\t-x:\tthe program to popen() for each received mail\n",
           "\n[ ", AM_VERSION, " ]\n",
           NULL)) {
    __write2(tmpstring);
    free(tmpstring);
  }
  exit(0);
}
