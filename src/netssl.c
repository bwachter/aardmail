#if (defined HAVE_SSL) || (defined HAVE_MATRIXSSL)
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "network.h"
#include "aardmail.h"
#include "aardlog.h"

#ifdef HAVE_SSL

int netsslread(SSL *ssl_handle, char *buf, int len){
	int i=0;
	while (i<1){
		i=SSL_read(ssl_handle, buf, len);
		switch (i){
		case SSL_ERROR_WANT_READ:
			continue;
		default:
			return i;
		}
	}
	return i;
}

int netsslwrite(SSL *ssl_handle, char *buf, int len){
	int i=0;
	while (i<1){
		i=SSL_write(ssl_handle, buf, len);
		switch (i){
		case SSL_ERROR_WANT_WRITE:
			continue;
		default: 
			return i;
		}
	}
	return i;
}

static int provide_client_cert(SSL *ssl, X509 **cert, EVP_PKEY **pkey)
{
	(void)ssl;
	(void)cert;
	(void)pkey;
  logmsg(L_ERROR, F_SSL, "The server requested a client certificate, but you did not specify one\n", NULL);
  return 0;
}

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

	if (strcmp(am_sslkey, "")){
		if ((SSL_CTX_use_certificate_chain_file(ctx, am_sslkey))!=1){
			logmsg(L_ERROR, F_SSL, "problem with SSL certfoo", NULL);
			am_sslconf ^= AM_SSL_USETLS; 
			return  -1;
		} 
		if ((SSL_CTX_use_PrivateKey_file(ctx, am_sslkey, SSL_FILETYPE_PEM))!=1){
			logmsg(L_ERROR, F_SSL, "problem with SSL certfoo", NULL);
			am_sslconf ^= AM_SSL_USETLS; 
			return  -1;
		}
	} else
		SSL_CTX_set_client_cert_cb(ctx, provide_client_cert);

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

#ifdef HAVE_MATRIXSSL
int sslRead(sslConn_t *cp, char *buf, int len){
}

// sslDoHandshake, sslConnect are losely based on matrixssl-examples
sslConn_t *sslDoHandshake(sslConn_t *conn){
	char buf[1024];
	int bytes, status, rc;

	conn->insock.size=1024;
	conn->insock.start = conn->insock.end = conn->insock.buf = 
		(unsigned char *)malloc(conn->insock.size);
	conn->outsock.size = 1024;
	conn->outsock.start = conn->outsock.end = conn->outsock.buf =
		(unsigned char *)malloc(conn->outsock.size);
	conn->inbuf.size = 0;
	conn->inbuf.start = conn->inbuf.end = conn->inbuf.buf = NULL;

	bytes = matrixSslEncodeClientHello(conn->ssl, &conn->outsock, 0);
	if (bytes < 0){
		goto error;
	}

	if (psSocketWrite(conn->fd, &conn->outsock) < 0) {
		logmsg(L_ERROR, F_SSL, "socketWrite() failed", NULL);
		goto error;
	}
	conn->outsock.start = conn->outsock.end = conn->outsock.buf;

 readMore:
	rc = sslRead(conn, buf, sizeof(buf));
	if (rc == 0){
		if (matrixSslHandshakeIsComplete(conn->ssl) == 0){
			goto readMore;
		}
	} else if (rc > 0){
		logmsg(L_ERROR, F_SSL, "sslRead() got data during handshake", NULL);
		goto readMore;
	} else {
		logmsg(L_ERROR, F_SSL, "sslRead() error in sslDoHandshake", NULL);
		goto error;
	}
	return conn;

 error:
	sslFreeConnection(&conn);
	return NULL;
}

int sslConnect(sslConn_t **cpp, int fd){
	sslConn_t *conn;

	conn=calloc(sizeof(sslConn_t), 1);
	conn->fd=fd;
	if (matrixSslNewSession(&conn->ssl, NULL, NULL, 0) < 0) {
		sslFreeConnection(&conn);
		return -1;
	}

	*cpp=sslDoHandshake(conn); // try handshake with any cipher suite
	if (*cpp==NULL){
		return -1;
	}
	return 0;
}

int netsslread(sslConn_t *ssl_handle, char *buf, int len){
}

int netsslwrite(sslConn_t *ssl_handle, char *buf, int len){
}

int netsslstart(int sd){
	int err;

	if (matrixSslOpen() < 0) {
		logmsg(L_ERROR, F_SSL, "matrixSslOpen() failed", NULL);
		am_sslconf ^= AM_SSL_USETLS; // remove usetls-bits
		return  -1;
	}

	if ((err = sslConnect(&ssl, sd))<0) {
		logmsg(L_ERROR, F_SSL, "sslConnect() failed", NULL);
		am_sslconf ^= AM_SSL_USETLS; // remove usetls-bits
		return  -1;
	}
}
#endif

#endif
