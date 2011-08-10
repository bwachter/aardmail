
.PHONY: clean install upload deb maintainer-deb rpm check dep deb ibaard-clean dist 

all: $(ALL)

$(SRCDIR)/version.h: CHANGES
	$(Q)echo "-> $@"
	$(Q)printf "#ifndef AM_VERSION_H\n#define AM_VERSION_H\n#define AM_VERSION \"" > $@
	$(Q)printf $(VERSION) >> $@
	$(Q)printf "; http://bwachter.lart.info/projects/aardmail/\"\n#endif\n" >> $@

aardmail.spec: CHANGES
	$(Q)echo "-> $@"
	$(Q)sed -i "s/Version:.*/Version: $(VERSIONNR)/" $@

clean: ibaard-clean 
	$(Q)echo "-> cleaning up"
	$(Q)$(RM) $(ALL) *.exe *.lib *.tds *.BAK $(OBJDIR)/*.{o,obj,lib} crammd5/*.{o,obj,lib} crammd5/*.o $(OBJDIR)/*.o $(SRCDIR)/version.h dyn-*.mk

distclean: clean
	$(Q)echo "-> cleaning up everything"
	$(Q)$(RM) -Rf ibaard Makefile.borland

dyn-conf.mk:
	$(Q)_IBAARD="";\
	if [ ! -z "$$IBAARD" ] && [ -d "$$IBAARD" ]; then\
	  _IBAARD=$$IBAARD; \
	  echo "-> including ibaard from $$IBAARD";\
	elif [ -d ibaard ]; then\
	  _IBAARD=ibaard;\
	  echo "-> including local libaard";\
	fi;\
	if [ ! -z $$_IBAARD ]; then\
	  printf "LDPATH+=-L$$_IBAARD\n";\
	  printf "INCLUDES+=-I$$_IBAARD/src\n";\
	  printf "ALL=libibaard.a $(ALL)\n";\
	  printf "_IBAARD=$$_IBAARD\n";\
	fi > $@

#fixme, need to generate dynamic build rules for ibaard

dyn-gmake.mk:
	$(Q)for i in 1; do \
	printf '$$(OBJDIR)/%%.o: $$(SRCDIR)/%%.c\n';\
	printf '\t$$(Q)echo "CC $$@"\n';\
	printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(CFLAGS) $$(INCLUDES) -c $$< -o $$@\n';\
	printf 'ifdef STRIP\n';\
	printf '\t$$(Q)$$(COMMENT) -$$(CROSS)$$(STRIP) $$@\n';\
	printf 'endif\n\n';\
	printf '%%.o: %%.c\n';\
	printf '\t$$(Q)echo "CC $$@"\n';\
	printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(CFLAGS) $$(INCLUDES) -c $$< -o $$@\n';\
	printf 'ifdef STRIP\n';\
	printf '\t$$(Q)$$(COMMENT) -$$(CROSS)$$(STRIP) $$@\n';\
	printf 'endif\n\n';\
	done > $@
	$(Q)for i in $(BD_BIN); do \
	printf "$$i: " ;\
	DEPS=`grep $$i targets | sed "s/\$$i//" | sed "s/\.exe//" | sed "s/\.c/\.o/g" | sed 's,src/,\$$(OBJDIR)/,g'`;\
	for j in $$DEPS; do printf "$$j "; done;\
	printf '\n\t$$(Q)echo "LD $$@"\n';\
	printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(LDFLAGS) -o bin/$$@ $$^ $$(LIBS)\n\n';\
	done >> $@
	$(Q)for i in $(BD_LIB); do \
	printf "$$i:" ;\
	DEPS=`grep $$i targets | sed "s/\$$i//" | sed "s/\.c/\.o/g" | sed 's,src/,\$$(OBJDIR)/,g'`;\
	for j in $$DEPS; do printf "$$j "; done;\
	printf '\n\t$$(Q)echo "AR $$@"\n';\
	printf '\t$$(Q)$$(CROSS)$$(AR) $$(ARFLAGS) $$@ $$^\n';\
	printf '\t$$(Q)$$(CROSS)$$(RANLIB) $$@\n';\
	printf '\n';\
	done >> $@

dyn-bsdmake.mk:
	$(Q)for i in 1; do \
	printf '.c.o:\n';\
	printf '\t$$(Q)echo "CC $$@"\n';\
	printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(CFLAGS) $$(INCLUDES) -c $$< -o $$@\n';\
	printf '.ifdef STRIP\n';\
	printf '\t$$(Q)$$(COMMENT) -$$(CROSS)$$(STRIP) $$@\n';\
	printf '.endif\n\n';\
	done > $@
	$(Q)for i in $(BD_BIN); do \
	printf "$$i: " ;\
	DEPS=`grep $$i targets | sed "s/\$$i//" | sed "s/\.exe//" | sed "s/\.c/\.o/g" | sed 's,src/,\$$(OBJDIR)/,g'`;\
	for j in $$DEPS; do printf "$$j "; done;\
	printf '\n\t$$(Q)echo "LD $$@"\n';\
	printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(LDFLAGS) -o bin/$$@ $$> $$(LIBS)\n\n';\
	done >> $@
	$(Q)for i in $(BD_LIB); do \
	printf "$$i:" ;\
	DEPS=`grep $$i targets | sed "s/\$$i//" | sed "s/\.c/\.o/g" | sed 's,src/,\$$(OBJDIR)/,g'`;\
	for j in $$DEPS; do printf "$$j "; done;\
	printf '\n\t$$(Q)echo "AR $$@"\n';\
	printf '\t$$(Q)$$(CROSS)$$(AR) $$(ARFLAGS) $$@ $$>\n';\
	printf '\t$$(Q)$$(CROSS)$$(RANLIB) $$@\n\n';\
	done >> $@

Makefile.borland:
	$(Q)for i in 1; do \
	printf "CC=bcc32\n";\
	printf "LD=bcc32\nRM=del /F\n";\
	printf "LDFLAGS=-tWC -w- -k- -q -O2 -lq -lc -lx -lGpd -lGn -lGl -lw-\n";\
	printf "CFLAGS=-w- -O2 -q\n";\
	printf "LIBS=-Libaard libaardmail.lib ibaard.lib ws2_32.lib\n";\
	printf "INCLUDES=-Iibaard/src\n";\
	printf "!ifdef SSL\n";\
	printf "SSLLIBS=ssleay32.lib libeay32.lib\n";\
	printf "SSLCFLAGS=-DHAVE_SSL\n";\
	printf "!endif\n";\
	printf "Q=@\n";\
	printf "ALL=ibaard/ibaard.lib libaardmail.lib aardmail-pop3c.exe aardmail-smtpc.exe aardmail-sendmail.exe aardmail-miniclient.exe\n";\
	printf 'OBJDIR=src\\\\\n';\
	printf 'SRCDIR=src\\\\\n';\
	printf ".PHONY: clean\n";\
	printf 'all: $$(ALL)\n';\
	done > $@
	$(Q)for i in 1; do \
	printf '.c.obj:\n';\
	printf '\t$$(Q)echo "CC $$@"\n';\
	printf '\t$$(Q)$$(CC) $$(CFLAGS) $$(INCLUDES) $$(SSLCFLAGS) -o$$@ -c $$<\n';\
	printf '\n';\
	done >> $@
	$(Q)for i in $(BD_BIN); do \
	printf "$$i.exe: " ;\
	DEPS=`grep $$i targets | sed "s/\$$i//" | sed "s/\.exe//" | sed "s/\.c/\.obj/g" | sed 's,src/,\$$(OBJDIR)/,g'`;\
	for j in $$DEPS; do printf "$$j "; done;\
	printf '\n\t$$(Q)echo "LD $$@"\n';\
	printf '\t$$(Q)$$(LD) $$(LDFLAGS) -e$$@ $$(LIBS) $$(SSLLIBS) $$**\n\n';\
	done >> $@
	$(Q)for i in $(BD_LIB); do \
	TARGET=`echo $$i | sed 's/\.a/\.lib/'`;\
	printf "$$TARGET: " ;\
	DEPS=`grep $$i targets | sed "s/\$$i//" | sed "s/\.c/\.obj/g" | sed "s,src/version\.h,," | sed 's,src/,\$$(OBJDIR),g'`;\
	for j in $$DEPS; do printf "$$j "; done;\
	printf '\n\t$$(Q)echo "TLIB $$@"\n';\
	printf '\t$$(Q)tlib $$(@F) /a $$**\n\n';\
	done >> $@
	$(Q)for i in 1; do \
	printf 'ibaard/ibaard.lib:\n';\
	printf '\t$$(Q)cd ibaard\n';\
	printf '\t$$(Q)make -f Makefile.borland\n';\
	printf '\t$$(Q)cd ..\n';\
	printf '\nclean:\n';\
	printf '\t$$(RM) *.exe *.lib *.tds src\*.obj\n';\
	printf '\t$$(Q)cd ibaard\n';\
	printf '\t$$(Q)make -f Makefile.borland clean\n';\
	done >> $@

libibaard.a:
	$(Q)echo "-> $@"
	$(Q)cd $(_IBAARD) && $(MAKE) DIET="$(DIET)" SSL="$(SSL)" DEV="$(DEV)" BROKEN="$(BROKEN)" WIN32="$(WIN32)" DEBUG="$(DEBUG)" CFLAGS="$(CFLAGS)"
	$(Q)cp $(_IBAARD)/libibaard.a .

ibaard-clean: 
	$(Q)echo "-> cleaning up libaard"
	$(Q)cd $(_IBAARD) && $(MAKE) clean

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(MANDIR)/man1
	install -d $(DESTDIR)$(MANDIR)/man3
	install -m 755 bin/* $(DESTDIR)$(BINDIR)
	install -m 644 $(BD_LIB) $(DESTDIR)$(LIBDIR)
	install -m 644 doc/man/man1/*.1 $(DESTDIR)$(MANDIR)/man1
	install -m 644 doc/man/man3/*.3 $(DESTDIR)$(MANDIR)/man3

dist: Makefile.borland $(SRCDIR)/version.h
	$(Q)echo "building archive ($(VERSION).tar.bz2)"
	git-archive-all.sh --format tar --prefix $(VERSION)/  $(VERSION).tar
	gzip -f $(VERSION).tar
	rm -f $(VERSION).zip
	tar2zip $(VERSION).tar.gz

upload: dist
	scp $(VERSION).* bwachter@lart.info:/home/bwachter/public_html/projects/download/snapshots

maintainer-deb: dist
	$(Q)tar xf $(VERSION).tar.gz && \
	  cd $(VERSION) && ./debchangelog && \
	  dpkg-buildpackage -rfakeroot
	$(Q)cd .. && rm -Rf $(VERSION)

deb: dist
	$(Q)tar xf $(VERSION).tar.gz && \
	  cd $(VERSION) && ./debchangelog && \
	  dpkg-buildpackage -rfakeroot -us -uc
	$(Q)cd .. && rm -Rf $(VERSION)

rpm: dist
	$(Q)if [ ! -z "`git status -s aardmail.spec`" ]; then \
	  echo "aardmail.spec updated, but not comitted. Can't continue."; \
	  false; \
	fi
	$(Q)rpmbuild -ta $(VERSION).tar.gz

help:
	$(Q)echo "Variables for building:"
	$(Q)echo -e "SSL=0|1\t\tenable/disable SSL support and link against OpenSSL or any"

	$(Q)echo -e "\t\tcompatible library. Default is 0."
	$(Q)echo -e "DEBUG=0|1\tenable/disable debug build/stripping binaries. Default is 0."
	$(Q)echo -e "WIN32=0|1\tenable/disable build for Windows. Adds .exe to the binaries"
	$(Q)echo -e "\t\tand links against libws2_32, libwsock32 and libgdi32.  Default is 0."
	$(Q)echo -e "CROSS=\t\tset a cross compiler prefix, for example i386-pc-mingw32-."
	$(Q)echo -e "\t\tDefault is unset."
	$(Q)echo -e "DIET=\t\tset the diet-wrapper if you want to link against dietlibc."
	$(Q)echo -e "\t\tDefault is unset."
