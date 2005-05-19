# set up some basic programs
#DIET=`which diet`
CROSS=
CC=gcc
LD=gcc
AR=ar
RM=/bin/rm -f
INSTALL=install

# set up some basic flags
ifneq ($(DEBUG),)
CFLAGS=-g -Wall -W -pipe -Os -D_LINUX_SOURCE
LDFLAGS=-g
else
CFLAGS=-Wall -W -pipe  -Os -D_LINUX_SOURCE
LDFLAGS=-s
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
ALL=miniclient$(EXE) aardmail-pop3c$(EXE)

# you should not need to touch anything below this line

OBJDIR=obj
SRCDIR=src
PREFIX?=/usr
.PHONY: clean 

all: $(ALL)

aardmail-pop3c$(EXE): $(OBJDIR)/pop3c.o $(OBJDIR)/cat.o \
	$(OBJDIR)/aardlog.o $(OBJDIR)/network.o $(OBJDIR)/netssl.o
	$(Q)echo "LD $@"
	$(Q)$(DIET) $(CROSS)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

miniclient$(EXE): $(OBJDIR)/network.o $(OBJDIR)/netssl.o $(OBJDIR)/miniclient.o \
	$(OBJDIR)/aardlog.o $(OBJDIR)/cat.o
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
