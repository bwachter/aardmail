#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#ifdef __WIN32__
#include <getopt.h>
#include <windows.h>
#include <winbase.h>
#include <io.h>
#include <ws2tcpip.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "aardmail.h"
#include "aardlog.h"
#include "authinfo.h"
#include "network.h"
#include "cat.h"
#include "maildir.h"

static struct {
	char *maildir;
	int keepmail;
} smtpc;

static void smtpc_usage(char *program);
static int smtpc_oksendline(int sd, char *msg);
static int smtpc_connectauth(authinfo *auth);
static int smtpc_session(int sd);
static int smtpc_quitclose(int sd);

static int smtpc_connectauth(authinfo *auth){
	int i, sd;
	char buf[1024];
	char myhost[NI_MAXHOST];

	if (gethostname(myhost, NI_MAXHOST)==-1){
		logmsg(L_WARNING, F_GENERAL, "unable to get hostname, setting to localhost.localdomain", NULL);
		strcpy(myhost, "localhost.localdomain");
	}

	if ((sd=netconnect(auth->machine, auth->port)) == -1)
		return -1;
	if ((i=netreadline(sd, buf)) == -1)
		return -1;
	else logmsg(L_INFO, F_NET, buf, NULL);

#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
	// FIXME, reconnect on error if ALLOWPLAIN is set
	// check if we have to use starttls. abort if USETLS is already set
	if ((am_sslconf & AM_SSL_STARTTLS) && !(am_sslconf & AM_SSL_USETLS)){
		if ((smtpc_oksendline(sd, "starttls\r\n")) == -1)
			return -1;

		am_sslconf ^= AM_SSL_USETLS;
		if ((i=netsslstart(sd))) {
			logmsg(L_ERROR, F_SSL, "unable to open tls-connection using starttls", NULL);
			smtpc_quitclose(sd);
			close(sd);
			return -1;
		} 
	}
#endif
	if ((smtpc_oksendline(sd, cati("helo ", myhost, "\r\n", NULL)))==-1)
		return -1;
	else return sd;

	// FIXME, add cram-md5-foo and authentificate, if wanted

	return -1; // we should'nt get here...
}

static int smtpc_session(int sd){
}

static int smtpc_quitclose(int sd){
	if ((smtpc_oksendline(sd, "quit\r\n")) == -1)
		return -1;
	return (close(sd));
}

static int smtpc_oksendline(int sd, char *msg){
	char buf[MAXNETBUF];
	int i;

	logmsg(L_INFO, F_NET, "> ", msg, NULL);
	if ((i=netwriteline(sd, msg)) == -1){
		logmsg(L_ERROR, F_NET, "unable to write line to network: ", strerror(errno), NULL);
		return -1;
	}
	if ((i=netreadline(sd, buf)) == -1){
		logmsg(L_ERROR, F_NET, "unable to read line from network: ", strerror(errno), NULL);
		return -1;
	}
	logmsg(L_INFO, F_NET, "< ", buf, NULL);
	if (!strncmp(buf, "2", 1))
		return 0;
	logmsg(L_ERROR, F_NET, "bad response: '", buf, "' after '", msg, "' from me", NULL); 
	return -1;
}

int main(int argc, char **argv){
	int c, sd;
	authinfo defaultauth;

#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
	while ((c=getopt(argc, argv, "b:c:df:h:lm:p:r:s:tu:v:x:")) != EOF){
#else
	while ((c=getopt(argc, argv, "b:dh:m:p:r:s:u:v:x:")) != EOF){
#endif
		switch(c){
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
			case 2: am_sslconf=AM_SSL_USETLS & AM_SSL_ALLOWPLAIN; break;
			case 3: am_sslconf=AM_SSL_STARTTLS; break;
			case 4: am_sslconf=AM_SSL_STARTTLS & AM_SSL_ALLOWPLAIN; break;
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
			smtpc.maildir = strdup(optarg);
			break;
		case 'p':
			logmsg(L_WARNING, F_GENERAL, "do not use -p password, it's unsecure. use .authinfo", NULL);
			strncpy(defaultauth.password, optarg, AM_MAXPASS);
			break;
		case 's':
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

	if (maildir_init(NULL, ".spool", 0)==-1){
		logmsg(L_ERROR, F_GENERAL, "unable to retrieve mails in spool", NULL);
		return -1;
	}

	if (!authinfo_init())
		if (authinfo_lookup(&defaultauth)==-1)
			logmsg(L_WARNING, F_GENERAL, "no record found in authinfo", NULL);

	if (!strcmp(defaultauth.port,""))
		strcpy(defaultauth.port, "25");

	logmsg(L_INFO, F_GENERAL, "connecting to machine ", defaultauth.machine, ", port ", defaultauth.port, NULL);
	if (strcmp(defaultauth.login, "")) logmsg(L_INFO, F_GENERAL, "using login-name: ", defaultauth.login, NULL);
	if (strcmp(defaultauth.login, "")) logmsg(L_INFO, F_GENERAL, "using password: yes", NULL);

	if ((sd=smtpc_connectauth(&defaultauth))==-1) exit(-1);
	if (smtpc_session(sd)==-1) exit(-1);
	if (smtpc_quitclose(sd)==-1) exit(-1);
	return 0;
}

static void smtpc_usage(char *program){
	char *tmpstring=NULL;
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
	if (!cat(&tmpstring, "Usage: ", program, " [-b program] [-c option] [-d] [-f certificate]\n",
					 "\t\t-h hostname [-l] [-m maildir] [-n number] [-p password]\n",
					 "\t\t[-s service] [-t] [-u user] [-v level]\n",
#else
	if (!cat(&tmpstring, "Usage: ", program, " [-b program] [-d] -h hostname [-m maildir] [-p password]\n",
					 "\t\t[-s service] [-t] [-u user]","\n",
#endif
					 "\t-b:\tonly send mail if program exits with zero status\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
					 "\t-c:\tcrypto options. Options may be: 0 (off), 1 (tls, like -t),\n",
					 "\t\t2 (tls, fallback to plain on error), 3 (starttls, no fallback)\n",
					 "\t\tand 4 (starttls, fallback to plain on error)\n",
#endif
					 "\t-d:\tdon't delete mail after sending (default is to delete)\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
					 "\t-f:\tthe certificate file to use for authentification\n",
#endif
					 "\t-h:\tspecify the hostname to connect to\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
					 "\t-l:\tuse starttls, exit on error (like -c 3)\n",
#endif
					 "\t-m:\tthe maildir for spooling; default is ~/Maildir/.spool\n",
					 "\t-p:\tthe password to use. Don't use this option.\n",
					 "\t-s:\tthe service to connect to. Must be resolvable if non-numeric.\n",
#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
					 "\t-t:\tuse tls. If tls is not possible the program will exit (like -c 1)\n",
#endif
					 "\t-u:\tthe username to use. You usually don't need this option.\n",
					 "\t-v:\tset the loglevel, valid values are 0 (no logging), 1 (deadly),\n",
					 "\t\t2 (errors, default), 3 (warnings) and 4 (info, very much)\n",
					 NULL)) {
		__write2(tmpstring);
		free(tmpstring);
	}
	exit(0);
}
