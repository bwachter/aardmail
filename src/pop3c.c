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
#include <ibaard_authinfo.h>
#include <ibaard_fs.h>
#include <ibaard_maildir.h>

#include "aardmail.h"

/// Structure holding runtime configuration values for pop3c
static struct {
    /// Path to the local MDA; if unset, spool to Maildir
    char *pipeto;
    /// Path to the users maildir
    char *maildir;
    /// Flag to indicate if mail should be deleted after retrieval
    int keepmail;
    /// The number of messages on the server
    int msgcount;
    /// Limit number of mails to retrieve
    int onlyget;
} pop3c;

#if (defined(__WIN32__)) || (defined _BROKEN_IO)
#define FDTYPE FILE*
#define FDINVAL NULL
#else
/// Type of file descriptors on this platform
#define FDTYPE int
/// Return value of open functions on this platform
#define FDINVAL -1
#endif

/// The unique name of a file inside a Maildir (i.e., where pop3c spools to)
char *uniqname;
/// The hostname or IP address pop3c binds to, if specifiedsm
char *bindname=NULL;
/// Generic buffer to temporary copy strings into
static char *tmpstring=NULL;

/** End pop3 session by writing 'quit' to specified file descriptor
 *
 * @param sd The filedescriptor to write to
 * @return -1 on error, close(sd) on success
 */
static int pop3c_quitclose(int sd);

/** Print usage information and exit()
 *
 * @param program path to the program to prefix help with (usually argv[0]
 */
static void pop3c_usage(char *program);

/** Send a command expecting a one-line response to sd
 *
 * @param sd The filedescriptor to write to
 * @param msg The command to send
 * @return -1 on error, 0 on success (=server responded with +OK)
 */
static int pop3c_oksendline(int sd, char *msg);

/** Ask the server for the number of messages (STAT)
 *
 * @param sd The servers file descriptor
 * @return -1 on error, the number of messages on success
 */
static int pop3c_getstat(int sd);

/** Open the spool fd
 *
 * Depending on command line flags, open a file in a Maildir, or a pipe to a
 * local MDA.
 *
 * @return A file descriptor to the mail, or negative numbers on error (return
 *         values of popen(), mdopen() or am_open(), depending on configuration)
 */
static FDTYPE pop3c_openspool();

/** Close the fd for a mail
 *
 * Close and flush the fd, and -- for mails spooled into a Maildir -- move
 * the mail to the correct directory
 *
 * @param fd The file descriptor of the mail
 * @return 0 on success, negative numbers on error (return values of clase(),
 *         mdclose() or pclose(), depending on configuration)
 */
static int pop3c_closespool(FDTYPE fd);

/** Download one message from server and save to provided file descriptor
 *
 * @param sd The servers file descriptor
 * @param fd The file descriptor of the local mail file
 * @param size unused
 * @return -1 on error, the size of the mail on success
 */
static long pop3c_getmessage(int sd, FDTYPE fd, int size);

/** Connect and authenticate to a POP3 server
 *
 * Connect, set up SSL if required, and send user and password
 *
 * @param auth Pointer to an authinfo structure
 * @return -1 on error, file descriptor for the connection on success
 */
static int pop3c_connectauth(authinfo *auth);

/** Download message(s) from server
 *
 * Depending on command line switches this will download some or all messages
 * from the server, keeping or deleting it on the server.
 *
 * Messages are either saved in the local Maildir, or sent to a local MDA
 * through a pipe.
 *
 * This function only sets up the environment and takes care of opening
 * an fd to store the message, the dirty work gets done in pop3c_openspool(),
 * pop3c_getmessage() and pop3c_closespool()
 *
 * @param sd The file descriptor to use
 * @return -1 on error, 0 on success
 */
static int pop3c_session(int sd);



static int pop3c_oksendline(int sd, char *msg){
  char buf[MAXNETBUF];
  int i;

  if ((i=netwriteline(sd, msg)) == -1){
    logmsg(L_ERROR, F_NET, "unable to write line to network: ", strerror(errno), NULL);
    return -1;
  }
  if ((i=netreadline(sd, buf)) == -1){
    logmsg(L_ERROR, F_NET, "unable to read line from network: ", strerror(errno), NULL);
    return -1;
  }
  if (i==0){
    logmsg(L_ERROR, F_NET, "peer closed connection: ", strerror(errno), NULL);
    return -1;
  }
  if (!strncmp(buf, "+OK", 3))
    return 0;
  logmsg(L_ERROR, F_NET, "bad response: '", buf, "' after '", msg, "' from me", NULL);
  return -1;
}

static int pop3c_getstat(int sd){
  char *tmp, *tmp2;
  char buf[MAXNETBUF];
  int i;

  if ((i=netwriteline(sd,"stat\r\n")) == -1){
    logmsg(L_ERROR, F_NET, "unable to write line to network: ", strerror(errno), NULL);
    return -1;
  }
  if ((i=netreadline(sd, buf)) == -1){
    logmsg(L_ERROR, F_NET, "unable to read line from network: ", strerror(errno), NULL);
    return -1;
  }
  if (i==0){
    logmsg(L_ERROR, F_NET, "peer closed connection: ", strerror(errno), NULL);
    return -1;
  }
  if (strncmp(buf, "+OK", 3)){
    logmsg(L_ERROR, F_NET, "bad response: '", buf, "'after 'stat' from me", NULL);
    return -1;
  }

  for (tmp=buf+4,i=0;*tmp!=' ';tmp++,i++){}
  if((tmp2=malloc((i+1)*sizeof(char))) == NULL){
    logmsg(L_ERROR, F_GENERAL, "malloc() failed at pop3c_getstat()", NULL);
    return -1;
  }
  strncpy(tmp2, buf+4, i);
  tmp2[i] = '\0';
  logmsg(L_INFO, F_NET, "There are ", tmp2, " messages on server", NULL);
  return atoi(tmp2);
}

static FDTYPE pop3c_openspool(){
  FDTYPE fd;
  if (pop3c.pipeto){
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
    fd=popen(pop3c.pipeto, "w");
#else
    fd=am_pipe(pop3c.pipeto);
#endif
  } else
    fd=mdopen(NULL, &uniqname);

  if (fd == FDINVAL)
    logmsg(L_ERROR, F_GENERAL, "opening spool failed", NULL);
  return fd;
}

static int pop3c_closespool(FDTYPE fd){
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
  fflush(fd);
  if (pop3c.pipeto) {
    if(pclose(fd)==-1) return -1;
  } else mdclose(NULL, &uniqname, fd);
#else
  if (pop3c.pipeto){
    int status;
    close(fd);
    wait(&status);
    return status;
  } else
    mdclose(NULL, &uniqname, fd);
#endif
  return 0;
}

static long pop3c_getmessage(int sd, FDTYPE fd, int size){
  char *tmp;
  char *buf[MAXNETBUF];
  int i, delayrn=0;
  long fsize=0;
#ifdef __GNUC__
  (void) size;
#endif

  for(;;){
    if ((i=netreadline(sd, (char *)buf)) == -1) {
      logmsg(L_ERROR, F_NET, "unable to read line from network", NULL);
      return -1;
    }
    /*
      if (i==0){
      logmsg(L_ERROR, F_NET, "peer closed connection: ", strerror(errno), NULL);
      return -1;
      }
    */
    //fsize+=i;
    tmp=(char *)buf;
    if (!(strcmp((char *)buf, ".\r\n"))){
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
      fwrite(buf, 1, i-2, fd);
#else
      write(fd, buf, i-2);
#endif
      return(fsize);
    } else {
      if (delayrn){
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
        fwrite("\n", 1, 1, fd);
#else
        write(fd, "\n", 1);
#endif
        delayrn=0;
      }
      // we'll delay writing \r\n till we get here the next time in case of \r\n.\r\n as ending
      if (!strcmp((char *)buf, "\r\n")) delayrn=1;
      else {
        tmp=(char *)buf;
        if (!strncmp(tmp+i-1, "\r\n", 2)){
          strncpy(tmp+i-1, "\n", 1);
          i--;
        }
        if (!strncmp((char *)buf, ".", 1)){
          tmp=(char *)buf;
          tmp++;
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
          fwrite(tmp, 1, i, fd);
        } else
          fwrite(buf, 1, i+1, fd);
#else
        write(fd, tmp, i);
      } else
          write(fd, buf, i+1);
#endif
    }
  }
}
}

static int pop3c_connectauth(authinfo *auth){
  int i, sd;
  char buf[1024];

  // ugly hack to get rid of the `label defined but not used' warning when building without SSL
  goto connect;

  connect:
  if ((sd=netconnect2(auth->machine, auth->port, bindname)) == -1)
    return -1;
  if ((i=netreadline(sd, buf)) == -1)
    return -1;
  if (i==0){
    logmsg(L_ERROR, F_NET, "peer closed connection: ", strerror(errno), NULL);
    return -1;
  }

#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
  // check if we have to use starttls. abort if USETLS is already set
  if ((am_sslconf & AM_SSL_STARTTLS) && !(am_sslconf & AM_SSL_USETLS)){
    if ((pop3c_oksendline(sd, "stls\r\n")) == -1) {
      if (am_sslconf & AM_SSL_ALLOWPLAIN){
        pop3c_quitclose(sd);
        am_sslconf = 0;
        logmsg(L_WARNING, F_NET, "Reconnecting using plaintext (you allowed this!)", NULL);
        goto connect;
      } else return -1;
    }

    am_sslconf ^= AM_SSL_USETLS;
    if ((i=netsslstart(sd))) {
      logmsg(L_ERROR, F_SSL, "unable to open tls-connection using starttls", NULL);
      if (am_sslconf & AM_SSL_ALLOWPLAIN){
        pop3c_quitclose(sd);
        am_sslconf = 0;
        logmsg(L_WARNING, F_NET, "Reconnecting using plaintext (you allowed this!)", NULL);
        goto connect;
      } else return -1;
    }
  }
#endif

  if (!cat(&tmpstring, "user ", auth->login, "\r\n", NULL))
    if ((pop3c_oksendline(sd, tmpstring)) == -1)
      return -1;

#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
  // we never send a password if we got a ssl key
  if (strcmp(am_sslkey, "")) return sd;
#endif

  if (!cat(&tmpstring, "pass ", auth->password, "\r\n", NULL)){
    if ((pop3c_oksendline(sd, tmpstring)) == -1)
      return -1;
  }

  return sd;
}

static int pop3c_quitclose(int sd){
  if ((pop3c_oksendline(sd, "quit\r\n")) == -1)
    return -1;
  return (close(sd));
}

static int pop3c_session(int sd){
  FDTYPE fd;
  int i;
  char *pop3_msgnrbuf[1024];

  if ((pop3c.msgcount=pop3c_getstat(sd)) == -1)
    exit(-1);

  if (pop3c.onlyget == 0 || pop3c.onlyget > pop3c.msgcount) pop3c.onlyget = pop3c.msgcount;

  for (i=1;i<=pop3c.onlyget;i++){
    if (sprintf((char *)pop3_msgnrbuf, "%i", i)==0){
      logmsg(L_ERROR, F_GENERAL, "sprintf() failed", NULL);
      return -1;
    }
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
    if ((fd=pop3c_openspool())==NULL) return -1;
#else
    if ((fd=pop3c_openspool())==-1) return -1;
#endif
    //pop3c_getsize(sd, i);
    if (!cat(&tmpstring, "retr ", pop3_msgnrbuf, "\r\n", NULL)){
      int del_error=0;
      if ((pop3c_oksendline(sd, tmpstring)) == -1) return -1;
      if (pop3c_getmessage(sd, fd, 0) < 0) del_error=1;
      if (pop3c_closespool(fd)!=0 || del_error==1) {
        logmsg(L_ERROR, F_GENERAL, "Unable to save message", NULL);
        return -1;
      }
      if (!pop3c.keepmail) {
        if (cat(&tmpstring, "dele ", pop3_msgnrbuf, "\r\n", NULL))
          return -1;
        else
          if ((pop3c_oksendline(sd, tmpstring)) == -1)
            return -1;
      }
    }
  }
  return 0;
}


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
  am_sslconf = 0;
  netsslcacert(".aardmail");
  am_ssl_paranoid = L_DEADLY;
#endif

  while ((c=getopt(argc, argv,
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
                   "a:b:c:df:g:h:lm:n:p:r:s:tu:v:x:"
#else
                   "a:b:df:h:m:n:p:r:s:u:v:x:"
#endif
            )) != EOF){
    switch(c){
      case 'a':
        bindname = strdup(optarg);
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
    strcpy(defaultauth.port, "110");

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
           "\t\tand 4 (starttls, fallback to plain on error)\n",
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
