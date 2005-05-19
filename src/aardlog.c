#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>

#include "aardmail.h"
#include "aardlog.h"

static void logwrite (char *msg){
	char *tmp = strdup(msg);

	if (!strncasecmp(tmp+strlen(tmp)-2, "\r\n", 2))
		tmp[strlen(tmp)-2]='\0';
	__write1(tmp);
	free(tmp);
	return;
}

int logmsg(int loglevel, int facility, char *msg, ...) {
	(void) facility;
	va_list ap;
	char *tmp;
	int die=0;

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


