#ifdef HAVE_SSL
#include <errno.h>
#include <string.h>
#include "network.h"
#include "aardmail.h"
#include "aardlog.h"

int netsslstart(int sd){
	int err;
	SSL_CTX *ctx;
	//	X509 *server_cert;

	// we're not meant to use ssl, return
	if (!(am_sslconf & AM_SSL_USETLS)) return -1;

	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
	ctx = SSL_CTX_new(SSLv23_client_method());
	if (!ctx){
		logmsg(L_ERROR, F_SSL, "problem with SSL_CTX_new()", NULL);
		am_sslconf ^= AM_SSL_USETLS; // remove usetls-bits
		return  -1;
	}

	ssl = SSL_new(ctx);
	if (!ssl) {
		logmsg(L_ERROR, F_SSL, "ssl not working\n", NULL);
		am_sslconf ^= AM_SSL_USETLS;
		return -1;
	}
	SSL_set_fd(ssl, sd);

	if ((err = SSL_connect(ssl)) < 0){
		logmsg(L_ERROR, F_SSL, "error on SSL_connect(): ", strerror(errno), NULL);
		am_sslconf ^= AM_SSL_USETLS;
		return -1;
	} else
		am_sslconf |= AM_SSL_USETLS; // enable usetls
	logmsg(L_INFO, F_SSL, "SSL-connection using ", (SSL_get_cipher(ssl)), NULL);
	return 0;
}

#endif
