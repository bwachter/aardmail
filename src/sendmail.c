#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>

#include "aardmail.h"
#include "aardlog.h"
#include "cat.h"
#include "fs.h"
#include "maildir.h"
#include "version.h"

#define AM_SM_IGNORE_DOTS 1

char *uniqname;

int main(int argc, char **argv){
	int smcfg=0;
	int i=1, isbody=0, c;
	char buf[1024], from[1024], *mymaildir=NULL;

	while ((c=getopt(argc, argv, "d:f:F:o:tvV")) != EOF){
		switch(c){
		case 'd':
			loglevel(atoi(optarg));
			break;
		case 'f': // set the from: line. will be overwritten if there's a From:-line
			break;
		case 'F':
			break; // set the fullname 
		case 'o':
			if (optarg[0]=='i') smcfg|=AM_SM_IGNORE_DOTS; // ignore single dots
			break;
		case 't':
			//FIXME
			break;
		case 'v':
			loglevel(4);
			break;
		case 'V':
			__write1(cati("aardmail-sendmail ", AM_VERSION, "\n", NULL));
			exit(0);
		}
	}

#if (defined(__WIN32__)) || (defined _BROKEN_IO)
	FILE *fd;
#else
	int fd;
#endif
	if (maildirfind(NULL)){
		logmsg(L_ERROR, F_MAILDIR, "unable to find maildir", NULL);
		return -1;
	}
	cat (&mymaildir, maildirpath, "/.spool", NULL);
	fd=maildiropen(mymaildir, &uniqname);
#if (defined(__WIN32)) || (defined _BROKEN_IO)
	if (fd == NULL)

#else
	if (fd == -1)
#endif
		{
			logmsg(L_ERROR, F_GENERAL, "opening spool failed", NULL);
			return -1;
		}
	while (i>0){
		i=read(0, buf, 1024);
		if (!isbody){
			if (!strncasecmp(buf, "from:", 5)){
				logmsg(L_DEBUG, F_GENERAL, "Found from: ", buf, NULL);
			}
			if (!strncmp(buf, "\n", 1)) {
				isbody=1;
				if (!strncmp(from, "\0", 1)){
					logmsg(L_ERROR, F_GENERAL, "Unable to figure out From:", NULL);
					goto error;
				}
				// header time ;)
			}
		}
		if (!strncmp(buf+i-2, "\r\n", 2)) {
			logmsg(L_ERROR, F_GENERAL, "Having \\r\\n in the input is bad for your karma", NULL);
			goto error;
		}
		if (!strncmp(buf+i-2, ".\n", 2) && !(smcfg & AM_SM_IGNORE_DOTS)) break;
		filewrite(fd, buf, i);
	}
	return maildirclose(mymaildir, &uniqname, fd);
 error:
	logmsg(L_ERROR, F_GENERAL, "some stupid error occured", NULL);
	return -1;
}

