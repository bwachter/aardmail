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
Q=@
ARFLAGS=cru
#-pipe crashes on IRIX
CFLAGS=-Wall -W -Os -Iibaard/src
LDFLAGS=-s
DEBUG_CFLAGS=-g -Wall -W -Os
DEBUG_LDFLAGS=-g

LIBS=-L. -laardmail -Libaard -libaard
ALL=libcrammd5.a libaardmail.a aardmail-pop3c$(EXE) aardmail-miniclient$(EXE) aardmail-smtpc$(EXE) aardmail-sendmail$(EXE)

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
