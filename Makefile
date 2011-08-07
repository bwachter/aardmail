.include "system.mk"

VERSIONNR!=head -1 CHANGES|sed 's/:.*//'
VERSION=aardmail-$(VERSIONNR)
#CURNAME=$(notdir $(shell pwd))
OS!=uname

.ifdef DEBUG
CFLAGS=$(DEBUG_CFLAGS)
LDFLAGS=$(DEBUG_LDFLAGS)
.endif

.ifdef WIN32
LIBS+=$(WIN32_LIBS)
EXE=.exe
.else
.if $(OS) == SunOS
LIBS+=$(SOLARIS_LIBS)
.ifeq ($(DEBUG),)
STRIP=strip -x
.endif
.endif
.if $(OS) == IRIX64
STRIP=test
.ifdef DEBUG
CFLAGS=-Wall -W -Os $(INCLUDES)
LDFLAGS=-g
.else
CFLAGS=-g -Wall -W -Os $(INCLUDES)
.endif
.endif
.if $(OS) == FreeBSD
.endif
.endif

.ifdef BROKEN
CFLAGS+=$(BROKEN_CFLAGS)
.endif

.ifdef MATRIXSSL
LIBS+=$(MATRIX_LIBS)
CFLAGS+=$(MATRIX_CFLAGS)
.endif

.ifdef GNUTLS
LIBS+=$(GNUTLS_LIBS)
CFLAGS+=$(GNUTLS_CFLAGS)
.endif

.ifdef SSL
.if $(SSL) == 1
LIBS+=$(SSL_LIBS)
CFLAGS+=$(SSL_CFLAGS)
.endif
.endif

.ifdef DEV
CFLAGS+=$(DEV_CFLAGS)
.endif

.if exists(dyn-conf.mk)
.include "dyn-conf.mk"  
.else   
DEPSTAT= "You need to run 'make dep'\n"
.endif

.include "build.mk"

.if exists(dyn-bsdmake.mk)
.include "dyn-bsdmake.mk"  
.else   
DEPSTAT= "You need to run 'make dep'\n"
.endif

dep: $(SRCDIR)/version.h dyn-conf.mk dyn-bsdmake.mk aardmail.spec
	mkdir -p bin
