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

# set up some basic flags
VERSION=aardmail-$(shell head -1 CHANGES|sed 's/://')
CURNAME=$(notdir $(shell pwd))

ifneq ($(DEBUG),)
CFLAGS=-g -Wall -W -pipe -Os -D_LINUX_SOURCE
LDFLAGS=-g
else
CFLAGS=-Wall -W -pipe  -Os -D_LINUX_SOURCE
LDFLAGS=-s
endif

ifdef BROKEN
CFLAGS+=-D_BROKEN_IO
endif

ifdef MATRIXSSL
LIBS+=-lmatrixsslstatic
CFLAGS+=-DHAVE_SSL
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
endif

ARFLAGS=cru
Q=@
ALL=aardmail-miniclient$(EXE) aardmail-pop3c$(EXE) aardmail-smtpc$(EXE)

# you should not need to touch anything below this line

OBJDIR=obj
SRCDIR=src
PREFIX?=/usr
.PHONY: clean install tar rename upload

all: $(ALL)

aardmail-pop3c$(EXE): $(OBJDIR)/pop3c.o $(OBJDIR)/cat.o \
	$(OBJDIR)/aardlog.o $(OBJDIR)/network.o $(OBJDIR)/netssl.o \
	$(OBJDIR)/maildir.o $(OBJDIR)/authinfo.o $(OBJDIR)/fs.o 
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

aardmail-smtpc$(EXE): $(OBJDIR)/smtpc.o $(OBJDIR)/cat.o \
	$(OBJDIR)/aardlog.o $(OBJDIR)/network.o $(OBJDIR)/netssl.o \
	$(OBJDIR)/fs.o $(OBJDIR)/maildir.o $(OBJDIR)/authinfo.o 
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

aardmail-miniclient$(EXE): $(OBJDIR)/network.o $(OBJDIR)/netssl.o $(OBJDIR)/miniclient.o \
	$(OBJDIR)/aardlog.o $(OBJDIR)/cat.o $(OBJDIR)/maildir.o $(OBJDIR)/fs.o
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(Q)echo "CC $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(CFLAGS) -c $< -o $@
ifneq ($(DEBUG),)
	$(Q)echo "Don't stripping objects"
else
	$(Q)$(COMMENT) -$(CROSS)strip -x -R .comment -R .note $@
endif

clean:
	$(Q)echo "cleaning up"
	$(Q)$(RM) $(ALL) $(OBJDIR)/*.o

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(ALL) $(DESTDIR)$(BINDIR)

tar: clean rename
	$(Q)echo "building archive ($(VERSION).tar.bz2)"
	$(Q)cd .. && tar cvvf $(VERSION).tar.bz2 $(VERSION) --use=bzip2 --exclude CVS
	$(Q)cd .. && rm -Rf $(VERSION)

rename:
	$(Q)if test $(CURNAME) != $(VERSION); then cd .. && cp -a $(CURNAME) $(VERSION); fi

upload: tar
	scp ../$(VERSION).tar.bz2 bwachter@lart.info:/home/bwachter/public_html/projects/download/snapshots
