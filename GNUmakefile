include system.mk

VERSIONNR=$(shell head -1 CHANGES|sed 's/://')
VERSION=aardmail-$(shell head -1 CHANGES|sed 's/://')
CURNAME=$(notdir $(shell pwd))

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
CFLAGS=-Wall -W -Os 
LDFLAGS=-g
else
CFLAGS=-g -Wall -W -Os
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

ifdef SSL
LIBS+=$(SSL_LIBS)
CFLAGS+=$(SSL_CFLAGS)
endif

ifdef DEV
CFLAGS+=$(DEV_CFLAGS)
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

aardmail-miniclient$(EXE): libaardmail.a $(OBJDIR)/miniclient.o 
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

aardmail-pop3c$(EXE): libaardmail.a $(OBJDIR)/pop3c.o 
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

aardmail-sendmail$(EXE): libaardmail.a $(OBJDIR)/sendmail.o 
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

aardmail-smtpc$(EXE): libaardmail.a $(OBJDIR)/smtpc.o
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

libaardmail.a: $(SRCDIR)/version.h $(OBJDIR)/network.o $(OBJDIR)/netssl.o $(OBJDIR)/aardlog.o $(OBJDIR)/cat.o \
	$(OBJDIR)/aardmail.o $(OBJDIR)/maildir.o $(OBJDIR)/authinfo.o $(OBJDIR)/fs.o \
	$(OBJDIR)/kirahvi.o $(OBJDIR)/addrlist.o
	$(Q)echo "AR $@"
	$(Q)$(CROSS)$(AR) $(ARFLAGS) $@ $^

libcrammd5.a: crammd5/client_crammd5.o crammd5/hmacmd5.o crammd5/md5.o
	$(Q)echo "AR $@"
	$(Q)$(CROSS)$(AR) $(ARFLAGS) $@ $^