#DIET=`which diet`
CROSS=
CC=gcc
LD=gcc
AR=ar
RM=/bin/rm -f
INSTALL=install
DESTDIR=
BINDIR=/usr/bin
MANDIR=/usr/share/man
STRIP=

OBJDIR=src
SRCDIR=src
PREFIX?=/usr

#set up some flags for later use
Q?=@
ARFLAGS=cru
#-pipe crashes on IRIX
LIBS=-L. -laardmail -libaard
INCLUDES=

CFLAGS=-Wall -W -Os $(INCLUDES) 
LDFLAGS=-s
DEBUG_CFLAGS=-g -Wall -W -Os $(INCLUDES)
DEBUG_LDFLAGS=-g

BD_BIN=aardmail-pop3c$(EXE) aardmail-miniclient$(EXE) aardmail-smtpc$(EXE) aardmail-sendmail$(EXE)
BD_LIB=libcrammd5.a libaardmail.a

ALL=$(BD_LIB) $(BD_BIN)

SOLARIS_LIBS=-lresolv -lsocket
WIN32_LIBS=-lws2_32 -lwsock32 -lgdi32

MATRIX_LIBS=-lmatrixsslstatic -lpthread
MATRIX_CFLAGS=-DHAVE_MATRIXSSL

GNUTLS_LIBS=-lgnutls-openssl -lgnutls -ltasn1 -lgcrypt -lgpg-error -lz
GNUTLS_CFLAGS=-DHAVE_SSL

SSL_LIBS=-lssl -lcrypto
SSL_CFLAGS=-DHAVE_SSL

DEV_CFLAGS=-D_DEV
BROKEN_CFLAGS=-D_BROKEN_IO
