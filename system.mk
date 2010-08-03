#DIET=`which diet`
CROSS=
CC?=gcc
LD?=gcc
AR?=ar
RANLIB?=ranlib
RM=/bin/rm -f
INSTALL=install
DESTDIR=
BINDIR?=/usr/bin
LIBDIR?=/usr/lib
MANDIR?=/usr/share/man
STRIP=
MAKE?=make
SSL?=1

OBJDIR=src
SRCDIR=src
PREFIX?=/usr

#set up some flags for later use
Q?=@
ARFLAGS=cru
#-pipe crashes on IRIX
LDPATH=-L.
LIBS=$(LDPATH) -laardmail -libaard
INCLUDES=

WARN=-W -Wundef -Wno-endif-labels -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align -Wmissing-declarations -Wno-multichar
#-Wunreachable-code
#-Wextra will clash with gcc3
#-Wdeclaration-after-statement
CFLAGS?=-Wall -W -Os $(WARN)
LDFLAGS=-s
DEBUG_CFLAGS=-g -Wall -W -Os $(INCLUDES)
DEBUG_LDFLAGS=-g

BD_BIN=aardmail-pop3c$(EXE) aardmail-pop3d$(EXE) aardmail-miniclient$(EXE) aardmail-smtpc$(EXE) aardmail-sendmail$(EXE)
BD_LIB=libaardmail.a

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
