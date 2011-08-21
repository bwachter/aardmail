include system.mk

# Allow overwriting of some common configuration values
ifeq (config.mk,$(wildcard config.mk))
include config.mk
endif

include mk/config.mk

ifeq (project.mk,$(wildcard project.mk))
include project.mk
endif

NAME=aardmail
VERSIONNR=$(shell head -1 CHANGES|sed 's/:.*//')
MAJOR=$(shell echo $(VERSIONNR)|sed 's/~.*//;s/\..*//')
MINOR=$(shell echo $(VERSIONNR)|sed 's/~.*//;s/[0-9]*\.//;s/\.[0-9]*//')
RELEASE=$(shell echo $(VERSIONNR)|sed 's/~.*//;s/[0-9]*\.[0-9]*\.*//;s/^$$/0/;')
VERSION=$(NAME)-$(VERSIONNR)
CURNAME=$(notdir $(shell pwd))
MK_ALL=$$^
MK_INCLUDE=include

ifndef RELEASE
RELEASE=0
endif

ifdef DEBUG
CFLAGS=$(DEBUG_CFLAGS)
LDFLAGS=$(DEBUG_LDFLAGS)
endif

ifdef WIN32
LIBS+=$(WIN32_LIBS)
EXE=.exe
else
ifeq ($(shell uname),SunOS)
LIBS+=$(SOLARIS_LIBS)
ifeq ($(DEBUG),)
STRIP=strip -x
endif
endif
ifeq ($(shell uname),IRIX64)
STRIP=
ifdef DEBUG
CFLAGS=-Wall -W -Os $(INCLUDES)
LDFLAGS=-g
else
CFLAGS=-g -Wall -W -Os $(INCLUDES)
endif
endif
endif

ifdef BROKEN
CFLAGS+=$(BROKEN_CFLAGS)
endif

ifdef MATRIXSSL
LIBS+=$(MATRIX_LIBS)
CFLAGS+=$(MATRIX_CFLAGS)
endif

ifdef GNUTLS
LIBS+=$(GNUTLS_LIBS)
CFLAGS+=$(GNUTLS_CFLAGS)
endif

ifeq ($(SSL),1)
LIBS+=$(SSL_LIBS)
CFLAGS+=$(SSL_CFLAGS)
endif

ifdef DEV
CFLAGS+=$(DEV_CFLAGS)
endif

ifeq (dyn-conf.mk,$(wildcard dyn-conf.mk))
include dyn-conf.mk
endif

ifneq (dyn-gmake.mk,$(wildcard dyn-gmake.mk))
ALL=dep
endif

include build.mk mk/common-targets.mk mk/packaging-targets.mk

ifeq (dyn-gmake.mk,$(wildcard dyn-gmake.mk))
include dyn-gmake.mk
endif

dep: $(SRCDIR)/version.h dyn-conf.mk dyn-gmake.mk aardmail.spec
	$(Q)mkdir -p $(BD_BINDIR)
	$(MAKE)
