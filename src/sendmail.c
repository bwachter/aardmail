#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#ifdef __WIN32__
#include <getopt.h>
#else
#endif

#include "aardmail.h"
#include "aardlog.h"
#include "cat.h"
#include "maildir.h"

static void sendmail_usage(char *program);

char *uniqname;

int main(int argc, char **argv){
	int i=1;
	char buf[1024];
#if (defined(__WIN32__)) || (defined _BROKEN_IO)
	FILE *fd;
#else
	int fd;
#endif
	fd=maildiropen(NULL, &uniqname);
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
		if (!strncmp(buf+i-2, ".\n", 2)) break;
		write(fd, buf, i);
	}
	maildirclose(NULL, &uniqname, fd);
	return -1;
}

static void sendmail_usage(char *program){
	char *tmpstring=NULL;
	if (!cat(&tmpstring, "Usage: ", program, " [-b program] [-d] -h hostname [-m maildir] [-p password]\n",
					 "\t\t[-r number] [-s service] [-t] [-u user] [-x program]","\n",
					 "\t-b:\tonly fetch mail if program exits with zero status\n",
					 "\t-d:\tdon't delete mail after retrieval (default is to delete)\n",
					 "\t-h:\tspecify the hostname to connect to\n",
					 "\t-m:\tthe maildir for spooling; default (unless -x used) is ~/Maildir\n",
					 "\t-n:\tonly load number mails\n",
					 "\t-p:\tthe password to use. Don't use this option.\n",
					 "\t-r:\treconnect after number mails (see FAQ)\n",
					 "\t-s:\tthe service to connect to. Must be resolvable if non-numeric.\n",
					 "\t-u:\tthe username to use. You usually don't need this option.\n",
					 "\t-v:\tset the loglevel, valid values are 0 (no logging), 1 (deadly),\n",
					 "\t\t2 (errors, default), 3 (warnings) and 4 (info, very much)\n",
					 "\t-x:\tthe program to popen() for each received mail\n",
					 NULL)) {
		__write2(tmpstring);
		free(tmpstring);
	}
	exit(0);
}
