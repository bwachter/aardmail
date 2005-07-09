#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#ifdef __WIN32__
#ifdef _GNUC_
#include <getopt.h>
#else
#include "getopt.h"
#endif
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
#include "addrlist.h"
#include "network.h"
#include "cat.h"
#include "maildir.h"
#include "fs.h"

static struct {
	char *maildir;
	int keepmail;
} smtpc;

static void smtpc_usage(char *program);
static int smtpc_oksendline(int sd, char *msg, char *ok);
static int smtpc_connectauth(authinfo *auth);
static int smtpc_session(int sd, char **msg);
static int smtpc_quitclose(int sd);

static int smtpc_connectauth(authinfo *auth){
	int i, sd;
	char have_ehlo=1;
	char buf[1024];
	char myhost[NI_MAXHOST];

 connectauth:
	if (gethostname(myhost, NI_MAXHOST)==-1){
		logmsg(L_WARNING, F_GENERAL, "unable to get hostname, setting to localhost.localdomain", NULL);
		strcpy(myhost, "localhost.localdomain");
	}

	if ((sd=netconnect(auth->machine, auth->port)) == -1)
		return -1;
	if ((i=netreadline(sd, buf)) == -1)
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

	return sd;
}

static int smtpc_getaddr(addrlist **addrlist_storage, char *msg){
	char *ptr, isaddr=0, endaddr=0;
	int i, start=0;
	for (ptr=msg,i=0;*ptr!=NULL;*ptr++,i++){
		if (*ptr=='\n') {
			*ptr='\0';
			endaddr=1;
		}
		if (*ptr=='<'){
			start=i;
			continue;
		}
		if (*ptr=='>' && isaddr){
			*ptr='\0';
			endaddr=1;
		}
		if (*ptr=='@') {
			isaddr=1;
			continue;
		}
		if (*ptr==',' || *ptr==';' || *ptr==' ') {
			if (isaddr==0) start=i;
			else {
				*ptr='\0';
				endaddr=1;
			}
		}
		if (isaddr && endaddr){
			logmsg(L_INFO, F_GENERAL, "Address found: ", msg+start+1, NULL);
			addrlist_append(addrlist_storage, msg+start+1);
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
	addrlist *rcptlist=NULL, *fromlist=NULL, *addrptr;
	char **buf, **bufptr, *ptr, prevchar;
	int isheader=1;

	if ((buf=malloc(sizeof(char*)*(strlen(*msg)+2)))==NULL) {
		logmsg(L_ERROR, F_GENERAL, "malloc() failed", NULL);
		return -1;
	}

	// split the mail buffer into lines and search for adresses
	bufptr=buf;
	*bufptr++=*msg;
	for (ptr=*msg;*ptr;ptr++){
		if (!strncasecmp(ptr, "From:", 5)) smtpc_getaddr(&fromlist, ptr);
		if (!strncasecmp(ptr, "To:", 3)) smtpc_getaddr(&rcptlist, ptr);
		if (!strncasecmp(ptr, "BCC:", 4)) smtpc_getaddr(&rcptlist, ptr);
		if (!strncasecmp(ptr, "CC:", 3)) smtpc_getaddr(&rcptlist, ptr);
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
		if ((smtpc_oksendline(sd, cati("RCPT TO:<", addrptr->address, ">\r\n"), "2")) == -1)
			goto error;
	}

	if ((smtpc_oksendline(sd, "DATA\r\n", "3")) == -1)
		goto error;

	netwriteline(sd, "X-Broken-By: aardmail-smtpc (http://bwachter.lart.info/projects/aardmail/)\r\n");
	for (bufptr=buf;*bufptr!=NULL;bufptr++){
		if (!strncasecmp(*bufptr, "BCC:", 4)) continue;
		netwriteline(sd, *bufptr);
		netwriteline(sd, "\r\n");
	}
	if ((smtpc_oksendline(sd, ".\r\n", "250")) == -1)
		goto error;

	free(buf);
	return 0;
 error:
	logmsg(L_ERROR, F_GENERAL, "error in smtpc_session", NULL);
	free(buf);
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

	while (!strncmp(buf+3, "-", 1)){
		// continuation, FIXME -- do something useful with it
		i=netreadline(sd, buf);
	}
	
	if (!strncmp(buf, ok, strlen(ok)))
		return 0;
	logmsg(L_ERROR, F_NET, "bad response: '", buf, "' after '", msg, "' from me", NULL);
	return -1;
}

int main(int argc, char **argv){
	int c, sd;
	authinfo defaultauth;
	unsigned long len;
	char *mail=0, *mymaildir=NULL;
	DIR *dirptr;
	struct dirent *tmpdirent;

#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
	while ((c=getopt(argc, argv, "b:c:df:h:lm:p:r:s:tu:v:x:")) != EOF){
#else
	while ((c=getopt(argc, argv, "a:b:dh:m:p:r:s:u:v:x:")) != EOF){
#endif
		switch(c){
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
			logmsg(L_WARNING, F_GENERAL, "do not use -p password, it's insecure. use .authinfo", NULL);
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

	if (maildirfind(smtpc.maildir)){
		logmsg(L_ERROR, F_MAILDIR, "unable to find maildir", NULL);
		return -1;
	}

	if (smtpc.maildir != NULL && !strcmp(smtpc.maildir, maildirpath)) { 
		// smtpc.maildir not set or not usable, append .spool
		cat (&mymaildir, maildirpath, "/cur", NULL);
	} else {
		cat(&mymaildir, maildirpath, "/.spool/cur", NULL);
	}

	if ((dirptr=opendir(mymaildir))==NULL){
		logmsg(L_ERROR, F_MAILDIR, "unable to open maildir ", mymaildir, ": ", strerror(errno), NULL);
		return -1;
	}

	logmsg(L_INFO, F_GENERAL, "connecting to machine ", defaultauth.machine, ", port ", defaultauth.port, NULL);
	if (strcmp(defaultauth.login, "")) logmsg(L_INFO, F_GENERAL, "using login-name: ", defaultauth.login, NULL);
	if (strcmp(defaultauth.login, "")) logmsg(L_INFO, F_GENERAL, "using password: yes", NULL);

	//	if ((sd=smtpc_connectauth(&defaultauth))==-1) exit(-1);
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
		} else if (unlink(tmpdirent->d_name)==-1) {
			logmsg(L_ERROR, F_GENERAL, "unable to delete mail ", mymaildir, "/", tmpdirent->d_name, ": ", strerror(errno), NULL);
			exit(-1);
		}
	}
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
