# Be silent per default
Q?=@

# main source directory; we support different, though
SRCDIR?=src
BD_OBJ?=src

# where to put binaries (not libraries)
BD_BINDIR?=bin/
BD_LIBDIR?=lib/

STATIC_LIBRARY?=1
DYNAMIC_LIBRARY?=0

#DIET=
#CROSS=

# Set some default programs
CC?=gcc
LD?=gcc
AR?=ar
RANLIB?=ranlib
RM?=/bin/rm -f
INSTALL?=install
MAKE?=make

# Set default flags for programs
WARN?=-W -Wundef -Wno-endif-labels -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align -Wmissing-declarations -Wno-multichar -Wunreachable-code
ARFLAGS?=cru

CFLAGS?=-Wall -W -Os
# on old systems it might be required to use -D_SVID_SOURCE

DEFAULT_CFLAGS?=-D_GNU_SOURCE -fPIC $(WARN) -std=c99 -D_XPG6 -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=500
DEBUG_CFLAGS?=-g -Wall -W -O0 -fprofile-arcs -ftest-coverage
INCLUDES+=-I$(SRCDIR)

LDFLAGS?=-s
DEFAULT_LDFLAGS=
DEBUG_LDFLAGS=-g -fprofile-arcs
LDPATH+=-L$(BD_LIBDIR)
LIBS+=$(LDPATH)

CFLAGS+=$(DEFAULT_CFLAGS)
DEBUG_CFLAGS+=$(DEFAULT_CFLAGS)

LDFLAGS+=$(DEFAULT_LDFLAGS)
DEBUG_LDFLAGS+=$(DEFAULT_LDFLAGS)

# Installation options
#DESTDIR=
PREFIX?=/usr
BINDIR?=$(PREFIX)/bin
LIBDIR?=$(PREFIX)/lib
MANDIR?=$(PREFIX)/share/man
#STRIP=



# Common flags for SSL
MATRIX_LIBS=-lmatrixsslstatic -lpthread
MATRIX_CFLAGS=-DHAVE_MATRIXSSL

GNUTLS_LIBS=-lgnutls-openssl -lgnutls -ltasn1 -lgcrypt -lgpg-error -lz
GNUTLS_CFLAGS=-DHAVE_SSL

SSL_LIBS=-lssl -lcrypto
SSL_CFLAGS=-DHAVE_SSL
