#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#ifndef __WIN32__
#include <strings.h>
#endif

#include "aardmail.h"
#include "aardlog.h"

static int current_loglevel=L_ERROR;

static void logwrite (char *msg){
	char *tmp = strdup(msg);

	if (!strncasecmp(tmp+strlen(tmp)-2, "\r\n", 2))
		tmp[strlen(tmp)-2]='\0';
	__write1(tmp);
	free(tmp);
	return;
}

int loglevel(int loglevel){
	if (loglevel != 0)
		current_loglevel = loglevel;
	return current_loglevel; 
}

int logmsg(int loglevel, int facility, char *msg, ...) {
#ifdef __GNUC__
	(void) facility;
#endif
	va_list ap;
	char *tmp;
	int die=0;

	if (loglevel > current_loglevel) return 0;

	switch(loglevel){
	case L_DEADLY:
		__write1("[DEADLY]");
		die=1;
		break;
	case L_ERROR:
		__write1("[ERROR]");
		break;
	case L_WARNING:
		__write1("[WARNING]");
		break;
	case L_INFO:
		__write1("[INFO]");
		break;
	case L_DEBUG:
		__write1("[DEBUG]");
		break;
	default:
		__write1("[UNKNOWN]");
		break;
	}

	switch(facility){
	case F_GENERAL:
		__write1("[GENERAL] ");
		break;
	case F_NET:
		__write1("[NET] ");
		break;
	case F_SSL:
		__write1("[SSL] ");
		break;
	case F_MAILDIR:
		__write1("[MAILDIR] ");
		break;
	case F_AUTHINFO:
		__write1("[AUTHINFO] ");
		break;
	default:
		__write1("[UNKNOWN] ");
		break;
	}

	logwrite(msg);
	va_start(ap, msg);
	while ((tmp = va_arg(ap, char*)))
		logwrite(tmp);
	va_end(ap);
	__write1("\n");

	if (die) exit(-1);
	return 0;
}


