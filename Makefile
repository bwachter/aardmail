# set up some basic programs
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

# set up some basic flags
VERSIONNR=$(shell head -1 CHANGES|sed 's/://')
VERSION=aardmail-$(shell head -1 CHANGES|sed 's/://')
CURNAME=$(notdir $(shell pwd))

ifneq ($(DEBUG),)
CFLAGS=-g -Wall -W -pipe -Os
LDFLAGS=-g
else
CFLAGS=-Wall -W -pipe  -Os
LDFLAGS=-s
endif

ifdef BROKEN
CFLAGS+=-D_BROKEN_IO
endif

ifdef MATRIXSSL
LIBS+=-lmatrixsslstatic -lpthread
CFLAGS+=-DHAVE_MATRIXSSL
endif

ifdef GNUTLS
LIBS+=-lgnutls-openssl -lgnutls -ltasn1 -lgcrypt -lgpg-error -lz
CFLAGS+=-DHAVE_SSL
endif

ifdef SSL
LIBS+=-lssl -lcrypto
CFLAGS+=-DHAVE_SSL
endif

ifdef WIN32
LIBS+=-lws2_32 -lwsock32 -lgdi32
EXE=.exe
else
ifeq ($(shell uname),SunOS)
LIBS+=-lresolv -lsocket
ifeq ($(DEBUG),)
STRIP=strip -x
endif
endif
ifeq ($(shell uname),IRIX64)
ifeq ($(DEBUG),)
STRIP=
CFLAGS=-Wall -W -Os 
LDFLAGS=-g
else
STRIP=
CFLAGS=-g -Wall -W -Os
endif
endif
endif

ARFLAGS=cru
Q=@
ALL=libcrammd5.a aardmail-pop3c$(EXE)
ifndef WIN32
ALL+=aardmail-miniclient$(EXE)
endif
ifdef DEV
ALL+=aardmail-smtpc$(EXE) aardmail-sendmail$(EXE)
endif

# you should not need to touch anything below this line

OBJDIR=obj
SRCDIR=src
PREFIX?=/usr
.PHONY: clean install tar rename upload deb maintainer-deb

all: $(ALL)
 
aardmail-miniclient$(EXE): $(OBJDIR)/network.o $(OBJDIR)/netssl.o $(OBJDIR)/miniclient.o \
	$(OBJDIR)/aardlog.o $(OBJDIR)/cat.o $(OBJDIR)/maildir.o $(OBJDIR)/fs.o
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

aardmail-pop3c$(EXE): $(OBJDIR)/pop3c.o $(OBJDIR)/cat.o \
	$(OBJDIR)/aardlog.o $(OBJDIR)/network.o $(OBJDIR)/netssl.o \
	$(OBJDIR)/maildir.o $(OBJDIR)/authinfo.o $(OBJDIR)/fs.o \
	$(OBJDIR)/aardmail.o
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

aardmail-sendmail$(EXE): $(OBJDIR)/sendmail.o $(OBJDIR)/cat.o \
	$(OBJDIR)/aardlog.o \
	$(OBJDIR)/maildir.o $(OBJDIR)/authinfo.o $(OBJDIR)/fs.o \
	$(OBJDIR)/aardmail.o
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

aardmail-smtpc$(EXE): $(OBJDIR)/smtpc.o $(OBJDIR)/cat.o \
	$(OBJDIR)/aardlog.o $(OBJDIR)/network.o $(OBJDIR)/netssl.o \
	$(OBJDIR)/fs.o $(OBJDIR)/maildir.o $(OBJDIR)/authinfo.o \
	$(OBJDIR)/aardmail.o
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

libcrammd5.a: crammd5/client-crammd5.o crammd5/hmacmd5.o crammd5/md5.o
	$(Q)echo "AR $@"
	$(Q)$(CROSS)$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(Q)echo "CC $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(CFLAGS) -c $< -o $@
ifneq ($(STRIP),)
	$(Q)$(COMMENT) -$(CROSS)$(STRIP) $@
endif
%.o: %.c
	$(Q)echo "CC $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(CFLAGS) -c $< -o $@
ifneq ($(STRIP),)
	$(Q)$(COMMENT) -$(CROSS)$(STRIP) $@
endif

clean:
	$(Q)echo "cleaning up"
	$(Q)$(RM) $(ALL) $(OBJDIR)/*.o crammd5/*.o

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 $(ALL) $(DESTDIR)$(BINDIR)
	install -m 644 doc/man/*.1 $(DESTDIR)$(MANDIR)/man1

tar: clean rename
	$(Q)echo "building archive ($(VERSION).tar.bz2)"
	$(Q)cd .. && tar cvvf $(VERSION).tar.bz2 $(VERSION) --use=bzip2 --exclude CVS
	$(Q)cd .. && rm -Rf $(VERSION)

rename:
	$(Q)if test $(CURNAME) != $(VERSION); then cd .. && cp -a $(CURNAME) $(VERSION); fi

upload: tar
	scp ../$(VERSION).tar.bz2 bwachter@lart.info:/home/bwachter/public_html/projects/download/snapshots

maintainer-deb: rename
	$(Q)cd ../$(VERSION) && ./debchangelog && dpkg-buildpackage -rfakeroot
	$(Q)cd .. && rm -Rf $(VERSION)

deb: rename
	$(Q)cd ../$(VERSION) && ./debchangelog && dpkg-buildpackage -rfakeroot -us -uc
	$(Q)cd .. && rm -Rf $(VERSION)
