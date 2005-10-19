include system.mk

VERSIONNR=$(shell head -1 CHANGES|sed 's/://')
VERSION=aardmail-$(shell head -1 CHANGES|sed 's/://')
CURNAME=$(notdir $(shell pwd))

ifdef DEBUG
CFLAGS=DEBUG_CFLAGS
LDFLAGS=DEBUG_LDFLAGS
endif

ifdef WIN32
LIBS+=WIN32_LIBS
EXE=.exe
else
ifeq ($(shell uname),SunOS)
LIBS+=SOLARIS_LIBS
ifeq ($(DEBUG),)
STRIP=strip -x
endif
endif
ifeq ($(shell uname),IRIX64)
STRIP=
ifdef DEBUG
CFLAGS=-Wall -W -Os 
LDFLAGS=-g
else
CFLAGS=-g -Wall -W -Os
endif
endif
endif

ifdef BROKEN
CFLAGS+=BROKEN_CFLAGS
endif

ifdef MATRIXSSL
LIBS+=MATRIX_LIBS
CFLAGS+=MATRIX_CFLAGS
endif

ifdef GNUTLS
LIBS+=GNUTLS_LIBS
CFLAGS+=GNUTLS_CFLAGS
endif

ifdef SSL
LIBS+=SSL_LIBS
CFLAGS+=SSL_CFLAGS
endif

ifdef DEV
CFLAGS+=DEV_CFLAGS
endif

include build.mk

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(Q)echo "CC $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(CFLAGS) -c $< -o $@
ifdef $(STRIP)
	$(Q)$(COMMENT) -$(CROSS)$(STRIP) $@
endif
%.o: %.c
	$(Q)echo "CC $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(CFLAGS) -c $< -o $@
ifdef $(STRIP)
	$(Q)$(COMMENT) -$(CROSS)$(STRIP) $@
endif