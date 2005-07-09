#ifndef _AARDLOG_H
#define _AARDLOG_H

enum facilities {
	F_GENERAL,
	F_NET,
	F_SSL,
	F_FS,
	F_MAILDIR,
	F_AUTHINFO,
	F_ADDRLIST,
};

enum loglevel {
	L_UNSPEC,
	L_DEADLY,
	L_ERROR,
	L_WARNING,
	L_INFO,
	L_DEBUG,
};

int logmsg(int loglevel, int facility, char *msg, ...);
int loglevel(int loglevel);

#endif
