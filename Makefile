.include "system.mk"

VERSIONNR!=head -1 CHANGES|sed 's/://'
VERSION=aardmail-$(VERSIONNR)
#CURNAME=$(notdir $(shell pwd))
OS!=uname

.ifdef DEBUG
CFLAGS=DEBUG_CFLAGS
LDFLAGS=DEBUG_LDFLAGS
.endif

.ifdef WIN32
LIBS+=WIN32_LIBS
EXE=.exe
.else
.if $(OS) == SunOS
LIBS+=SOLARIS_LIBS
.ifeq ($(DEBUG),)
STRIP=strip -x
.endif
.endif
.if $(OS) == IRIX64
STRIP=test
.ifdef DEBUG
CFLAGS=-Wall -W -Os 
LDFLAGS=-g
.else
CFLAGS=-g -Wall -W -Os
.endif
.endif
.if $(OS) == FreeBSD
Q=
.endif
.endif

.ifdef BROKEN
CFLAGS+=BROKEN_CFLAGS
.endif

.ifdef MATRIXSSL
LIBS+=MATRIX_LIBS
CFLAGS+=MATRIX_CFLAGS
.endif

.ifdef GNUTLS
LIBS+=GNUTLS_LIBS
CFLAGS+=GNUTLS_CFLAGS
.endif

.ifdef SSL
LIBS+=SSL_LIBS
CFLAGS+=SSL_CFLAGS
.endif

.ifdef DEV
CFLAGS+=DEV_CFLAGS
.endif

.c.o:
	$(Q)echo "CC $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(CFLAGS) -c $< -o $@
.ifdef $(STRIP)
	$(Q)$(COMMENT) -$(CROSS)$(STRIP) $@
.endif

.include "build.mk"
