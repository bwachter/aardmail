/**
 * @file smtpc.c
 * @author Bernd Wachter <bwachter@lart.info>
 * @date 2005-2011
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#ifdef __WIN32__
#ifdef _GNUC_
#include <getopt.h>
#else
#include <ibaard_getopt.h>
#endif
#include <windows.h>
#include <winbase.h>
#include <io.h>
#include <ws2tcpip.h>
#else
#include <strings.h>
#include <sys/wait.h>
#include <unistd.h>
#include <getopt.h>
#endif

#include <ibaard_network.h>
#include <ibaard_log.h>
#include <ibaard_cat.h>
#include <ibaard_fs.h>
#include <ibaard_authinfo.h>
#include <ibaard_maildir.h>

#include "aardmail.h"
#include "addrlist.h"

/// Structure holding runtime configuration values for smtpc
static struct {
    /// The maildir to use
    char *maildir;
    /// Don't delete mail after sending, currently not used
    /// @todo implement
    int keepmail;
} smtpc;

/** Print usage information and exit()
 *
 * @param program path to the program to prefix help with (usually argv[0]
 */
static void smtpc_usage(char *program);

/** Send a command expecting a one-line response to sd
 *
 * @param sd The filedescriptor to write to
 * @param msg The command to send
 * @param ok The response to expect, or the beginning of it. For example, if ok
 *        is 220 only 220 will be accepted, if it is 2 all responses starting with
 *        2 are ok
 * @return -1 on error, 0 on success
 */
static int smtpc_oksendline(int sd, char *msg, char *ok);

/** Connect to remote server and optionally authenticate
 *
 * Greet server with EHLO or HELO, establish SSL connection if required,
 * perform AUTH PLAIN if requested and connection is secure.
 *
 * @param auth Pointer to authinfo structure containing login details
 * @return -1 on error, a file descriptor to the connection on success
 */
static int smtpc_connectauth(authinfo *auth);

/** Send one message by SMTP
 *
 * @param sd The file descriptor of the SMTP connection
 * @param msg The message to send
 * @return -1 on error, 0 on success
 */
static int smtpc_session(int sd, char **msg);

/** End smtp session by writing 'quit' to specified file descriptor
 *
 * @param sd The filedescriptor to write to
 * @return -1 on error, close(sd) on success (=server response starts with 2)
 */
static int smtpc_quitclose(int sd);

/// The hostname or IP address smtpc binds to, if specified
char *bindname=NULL;

static int smtpc_connectauth(authinfo *auth){
  int i, sd;
  char have_ehlo=1;
  char buf[1024];
  char myhost[NI_MAXHOST];

  connectauth:
  if (gethostname(myhost, NI_MAXHOST)==-1){
    logmsg(L_WARNING, F_GENERAL, "unable to get hostname, setting to localhost.localdomain", NULL);
    strncpy(myhost, "localhost.localdomain", NI_MAXHOST);
  }

  if ((sd=netconnect2(auth->machine, auth->port, bindname)) == -1)
    return -1;
  if ((i=netreadline(sd, buf)) == -1)
    return -1;
  if (i==0)
    return -1;

  // send HELO or EHLO, FIXME
  if (have_ehlo == 1){
    if ((smtpc_oksendline(sd, cati("EHLO ", myhost, "\r\n", NULL), "250"))==-1)
      if ((smtpc_oksendline(sd, cati("HELO ", myhost, "\r\n", NULL), "250"))==-1){
        have_ehlo=0;
        smtpc_quitclose(sd);
        goto connectauth;
      }
  } else {
    if ((smtpc_oksendline(sd, cati("HELO ", myhost, "\r\n", NULL), "250"))==-1) {
      smtpc_quitclose(sd);
      return -1;
    }
  }

#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
  // check if we have to use starttls. abort if USETLS is already set
  if ((am_sslconf & AM_SSL_STARTTLS) && !(am_sslconf & AM_SSL_USETLS)){
    if ((smtpc_oksendline(sd, "starttls\r\n", "2")) == -1){
      smtpc_quitclose(sd);
      if (am_sslconf & AM_SSL_ALLOWPLAIN) {
        logmsg(L_WARNING, F_NET, "Reconnecting using plaintext (you allowed this!)", NULL);
        am_sslconf = 0;
        goto connectauth;
      } else return -1;
    }

    am_sslconf ^= AM_SSL_USETLS;
    if ((i=netsslstart(sd))) {
      logmsg(L_ERROR, F_SSL, "unable to open tls-connection using starttls", NULL);
      if (am_sslconf & AM_SSL_ALLOWPLAIN){
        logmsg(L_WARNING, F_NET, "Reconnecting using plaintext (you allowed this!)", NULL);
        am_sslconf = 0;
        goto connectauth;
      } else return -1;
    }
  }
#endif

  // now that we might have a tls connection, check if we need to do password
  // authentication (cert auth is handled automatically by openssl callback)
  // FIXME: check for methods the server offers
  if (strcmp(auth->login, "")){
    // dirty hack, get the base64 hash from .authinfo
    if ((smtpc_oksendline(sd, cati("AUTH PLAIN ", auth->login, "\r\n", NULL), "235"))==-1){
      smtpc_quitclose(sd);
      return -1;
    }
  }

  return sd;
}

/** Find addresses and store them into an addrlist structure
 *
 * Typically this function gets called for individual lists for
 * to/from/cc/bcc, as soon as the message parsing function encounters
 * one of those headers.
 *
 * @param addrlist_storage Pointer to the addrlist structure to use
 * @param msg Text to search for addresses
 * @return Currently always returns 0
 */
static int smtpc_getaddr(addrlist **addrlist_storage, char *msg){
  char *ptr, isaddr=0, isquote=0, endaddr=0, buf[1024];
  int i, start=0, folding=0;
  for (ptr=msg,i=0;*ptr!='\0';ptr++,i++){
    if (folding && *ptr!=' ') break;
    if (folding) folding=0;
    if (*ptr=='\\'){
      isquote=1;
      continue;
    } else {
      // we could just check for @ here, but probably don't care about
      // _any_ quoted characters - \ is illegal in addresses unless in
      // double quotes. Double quotes are currently fully ignored (see
      // comment below)
      if (isquote){
        isquote=0;
        continue;
      }
      isquote=0;
    }
    if (*ptr=='\n'){
      endaddr=1;
      folding=1;
    }
    if (*ptr=='<'){
      start=i+1;
      continue;
    }
    if (*ptr=='>' && isaddr) endaddr=1;
    if (*ptr=='@') {
      isaddr=1;
      continue;
    }
    // TODO: those are allowed inside of quotes - though very rarely used.
    //       track quotes as well, and don't reset the start when inside of
    //       quotes
    if (*ptr==',' || *ptr==';' || *ptr==' ') {
      if (isaddr==0) start=i+1;
      else      endaddr=1;
    }
    if (isaddr && endaddr){
      strncpy(buf, msg+start, i-start);
      buf[i-start]='\0';
      logmsg(L_DEBUG, F_GENERAL, "Address found: ", buf, NULL);
      addrlist_append(addrlist_storage, buf);
      isaddr=endaddr=0;
      start=i;
      continue;
    }
    if (endaddr) {
      endaddr=0;
      start=i;
    }
  }
  return 0;
}

static int smtpc_session(int sd, char **msg){
  addrlist *rcptlist, *fromlist, *addrptr;
  char **buf=NULL, **bufptr, *ptr, prevchar='\n';
  int isheader=1;

  //    if ((*addrlist_storage = malloc(sizeof(addrlist))) == NULL);
  if ((rcptlist=malloc(sizeof(addrlist))) == NULL ||
      (fromlist=malloc(sizeof(addrlist))) == NULL){
    logmsg(L_ERROR, F_GENERAL, "Unable to malloc() memory for addrlist", NULL);
    goto error;
  }
  memset(rcptlist, 0, sizeof(addrlist));
  memset(fromlist, 0, sizeof(addrlist));

  if ((buf=malloc(sizeof(char*)*(strlen(*msg)+2)))==NULL) {
    logmsg(L_ERROR, F_GENERAL, "malloc() failed", NULL);
    return -1;
  }

  // split the mail buffer into lines and search for adresses
  bufptr=buf;
  *bufptr++=*msg;
  for (ptr=*msg;*ptr;ptr++){
    if (prevchar=='\n'){
      if (isheader && !strncasecmp(ptr, "From:", 5)) smtpc_getaddr(&fromlist, ptr);
      if (isheader && !strncasecmp(ptr, "To:", 3)) smtpc_getaddr(&rcptlist, ptr);
      if (isheader && !strncasecmp(ptr, "BCC:", 4)) smtpc_getaddr(&rcptlist, ptr);
      if (isheader && !strncasecmp(ptr, "CC:", 3)) smtpc_getaddr(&rcptlist, ptr);
    }
    if (*ptr=='\n') {
      if (prevchar=='\n')
        isheader=0;
      prevchar='\n';
      *ptr='\0';
      *bufptr++=ptr+1;
    } else prevchar=*ptr;
  } *bufptr++=NULL;

  if (fromlist==NULL){
    logmsg(L_ERROR, F_GENERAL, "No from-address found, aborting", NULL);
    goto error;
  }
  if (rcptlist==NULL){
    logmsg(L_ERROR, F_GENERAL, "No to-address found, aborting", NULL);
    goto error;
  }

  if ((smtpc_oksendline(sd, cati("MAIL FROM:<", fromlist->address, ">\r\n", NULL), "2")) == -1)
    goto error;

  for (addrptr=rcptlist;addrptr!=NULL;addrptr=addrptr->next){
    if ((smtpc_oksendline(sd, cati("RCPT TO:<", addrptr->address, ">\r\n", NULL), "2")) == -1)
      goto error;
  }

  if ((smtpc_oksendline(sd, "DATA\r\n", "3")) == -1)
    goto error;

  netwriteline(sd, "X-Broken-By: aardmail-smtpc (http://bwachter.lart.info/projects/aardmail/)\r\n");
  for (bufptr=buf;*bufptr!=NULL;bufptr++){
    if (!strncasecmp(*bufptr, "BCC:", 4)) continue;
    if (**bufptr=='.') netwriteline(sd, cati(".", *bufptr, "\r\n", NULL));
    else netwriteline(sd, cati(*bufptr, "\r\n", NULL));
  }
  if ((smtpc_oksendline(sd, ".\r\n", "250")) == -1)
    goto error;

  free(buf);
  addrlist_free(&rcptlist);
  addrlist_free(&fromlist);
  return 0;
  error:
  free(buf);
  addrlist_free(&rcptlist);
  addrlist_free(&fromlist);
  return -1;
}

static int smtpc_quitclose(int sd){
  if ((smtpc_oksendline(sd, "quit\r\n", "2")) == -1)
    return -1;
  return (close(sd));
}

static int smtpc_oksendline(int sd, char *msg, char *ok){
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

  while (!strncmp(buf+3, "-", 1)){
    // continuation, FIXME -- do something useful with it
    i=netreadline(sd, buf);
    if ((i==0)||(i==-1)) return -1;
  }

  if (!strncmp(buf, ok, strlen(ok)))
    return 0;
  logmsg(L_ERROR, F_NET, "bad response: '", buf, "' after '", msg, "' from me", NULL);
  return -1;
}

/** Main entry point
 *
 * @param argc Argument count
 * @param argv Argument array
 * @return -1 on error, 0 on success
 */
int main(int argc, char **argv){
  int c, sd;
  authinfo defaultauth;
  unsigned long len;
  char *mail=0, *mymaildir=NULL, *identity=NULL;
  DIR *dirptr;
  struct dirent *tmpdirent;

  memset(&defaultauth, 0, sizeof(authinfo));

#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
  am_sslconf = AM_SSL_USETLS;
  netsslcacert(".aardmail");
  am_ssl_paranoid = L_DEADLY;
#endif

  while ((c=getopt(argc, argv,
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
                   "a:b:c:df:g:h:i:lm:p:r:s:tu:v:x:"
#else
                   "a:b:dh:i:m:p:r:s:u:v:x:"
#endif
            )) != EOF){
    switch(c){
      case 'a':
        bindname = strdup(optarg);
        break;
      case 'b':
        if (am_checkprogram(optarg)!=0) {
          logmsg(L_INFO, F_GENERAL, "not sending because program evaluated to false", NULL);
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
        smtpc.keepmail = 1;
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
      case 'i':
        identity=strdup(optarg);
        break;
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
      case 'l':
        am_sslconf = AM_SSL_STARTTLS;
        break;
#endif
      case 'm':
        smtpc.maildir = strdup(optarg);
        break;
      case 'p':
        logmsg(L_WARNING, F_GENERAL, "do not use -p password, it's insecure. use .authinfo", NULL);
        strncpy(defaultauth.password, optarg, AM_MAXPASS);
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
    }
  }

  if (!strcmp(defaultauth.machine,""))
    smtpc_usage(argv[0]);

  if (!authinfo_init())
    if (authinfo_lookup(&defaultauth)==-1)
      logmsg(L_WARNING, F_GENERAL, "no record found in authinfo", NULL);

  if (!strcmp(defaultauth.port,""))
    strcpy(defaultauth.port, "465");

  if (mdfind(smtpc.maildir)){
    logmsg(L_ERROR, F_MAILDIR, "unable to find maildir", NULL);
    return -1;
  }

  if (smtpc.maildir != NULL && !strcmp(smtpc.maildir, maildirpath)) {
    cat (&mymaildir, maildirpath, "/new", NULL);
  } else {
    // smtpc.maildir not set or not usable, prepend .spool
    // check if we need to insert identity string
    if (identity)
      cat(&mymaildir, maildirpath, "/.spool/", identity, "/new", NULL);
    else
      cat(&mymaildir, maildirpath, "/.spool/new", NULL);
  }

  if ((dirptr=opendir(mymaildir))==NULL){
    logmsg(L_ERROR, F_MAILDIR, "unable to open maildir ", mymaildir, ": ", strerror(errno), NULL);
    return -1;
  }

  logmsg(L_INFO, F_GENERAL, "connecting to machine ", defaultauth.machine, ", port ", defaultauth.port, NULL);
  if (strcmp(defaultauth.login, "")) logmsg(L_INFO, F_GENERAL, "using login-name: ", defaultauth.login, NULL);
  if (strcmp(defaultauth.login, "")) logmsg(L_INFO, F_GENERAL, "using password: yes", NULL);

  if ((sd=smtpc_connectauth(&defaultauth))==-1) exit(-1);

  // FIXME for each mail do smtpc_session; don't exit on failure, just don't delete the mail
  for (tmpdirent=readdir(dirptr); tmpdirent!=NULL; tmpdirent=readdir(dirptr)){
    if (!strcmp(tmpdirent->d_name, ".")) continue;
    if (!strcmp(tmpdirent->d_name, "..")) continue;
    logmsg(L_DEBUG, F_GENERAL, "processing ", cati(mymaildir, "/", tmpdirent->d_name, NULL), NULL);
    if (openreadclose(cati(mymaildir, "/", tmpdirent->d_name, NULL), &mail, &len)){
      logmsg(L_ERROR, F_GENERAL, "error reading mail ", cati(mymaildir, "/", tmpdirent->d_name, NULL),
": ", strerror(errno), NULL);
      continue;
    }
    if (smtpc_session(sd, &mail)==-1) {
      // FIXME check how long the mail is in queue already, `send' warning
    } else if (unlink(cati(mymaildir, "/", tmpdirent->d_name, NULL))==-1) {
      logmsg(L_ERROR, F_GENERAL, "unable to delete mail ", mymaildir, "/", tmpdirent->d_name, ": ", strerror(errno), NULL);
      exit(-1);
    }
    free(mail);
    mail=0;
  }
  if (smtpc_quitclose(sd)==-1) exit(-1);
  return 0;
}

static void smtpc_usage(char *program){
  char *tmpstring=NULL;
  if (!cat(&tmpstring, "Usage: ", program,
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
           " [-b program] [-c option] [-d] [-f certificate]\n",
           "\t\t[-g certificate] -h hostname [-l] [-m maildir] [-n number]\n",
           "\t\t[-p password] [-s service] [-t] [-u user] [-v level]\n",
#else
           " [-b program] [-d] -h hostname [-m maildir] [-p password]\n",
           "\t\t[-s service] [-t] [-u user]","\n",
#endif
           "\t-a:\tbind to given address (hostnames will be resolved)\n",
           "\t-b:\tonly send mail if program exits with zero status\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
           "\t-c:\tcrypto options. Options may be: 0 (off), 1 (tls, like -t),\n",
           "\t\t2 (tls, fallback to plain on error), 3 (starttls, no fallback)\n",
           "\t\tand 4 (starttls, fallback to plain on error). Default is 1.\n",
#endif
           "\t-d:\tdon't delete mail after sending (default is to delete)\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
           "\t-f:\tthe certificate file to use for authentification\n",
           "\t-g:\tthe certificate file to use for verification\n",
#endif
           "\t-h:\tspecify the hostname to connect to\n",
           "\t-i:\tspecify the identity to use\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
           "\t-l:\tuse starttls, exit on error (like -c 3)\n",
#endif
           "\t-m:\tthe maildir for spooling; default is ~/Maildir/.spool\n",
           "\t-p:\tthe password to use. Don't use this option.\n",
           "\t-s:\tthe service to connect to. Must be resolvable if non-numeric.\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
           "\t-t:\tuse tls. If tls is not possible the program will exit (like -c 1)\n",
           "\t\tThis is the default setting.\n",
#endif
           "\t-u:\tthe username to use. You usually don't need this option.\n",
           "\t-v:\tset the loglevel, valid values are 0 (no logging), 1 (deadly),\n",
           "\t\t2 (errors, default), 3 (warnings) and 4 (info, very much)\n",
           "\n[ ", AM_VERSION, " ]\n",
           NULL)) {
    __write2(tmpstring);
    free(tmpstring);
  }
  exit(0);
}
