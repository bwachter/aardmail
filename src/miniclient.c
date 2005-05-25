#include <stdlib.h>

#include "aardmail.h"
#include "network.h"
#include "cat.h"

int main(int argc, char** argv){
	char *tmpstring=NULL;
	char buf[1024];
	int i, sd;
	if (argc!=3){
		cat(&tmpstring, "Usage: ", argv[0], " hostname servicename\n", NULL);
		__write2(tmpstring);
		cat(&tmpstring, "\t", argv[0], " lart.info smtp\n", NULL);
		__write2(tmpstring);
		cat(&tmpstring, "\t", argv[0], " lart.info 25\n", NULL);
		__write2(tmpstring);
		free(tmpstring);
		return -1;
	}

	//debuglevel=1;
#ifdef HAVE_SSL
	am_sslconf = AM_SSL_USETLS;	
#endif
	sd=netconnect(argv[1], argv[2]);
	i=netreadline(sd, buf);
	__write2(buf);
#ifdef HAVE_SSL
	//i=netstls(sd, "STLS");
#endif
	//	i=netreadline(sd, buf);
	//__write2(buf);
	return 0;  
}
