
.PHONY: clean install tar rename upload deb maintainer-deb check dep ibaard-clean ../ibaard-clean

all: $(ALL)

../ibaard/libibaard.a:
	$(Q)echo "-> $@"
	$(Q)make -C ../ibaard DIET=$(DIET) SSL=$(SSL) DEV=$(DEV) BROKEN=$(BROKEN) WIN32=$(WIN32) DEBUG=$(DEBUG)

ibaard/libibaard.a:
	$(Q)echo "-> $@"
	$(Q)make -C ibaard DIET=$(DIET) SSL=$(SSL) DEV=$(DEV) BROKEN=$(BROKEN) WIN32=$(WIN32) DEBUG=$(DEBUG)

$(SRCDIR)/version.h: 
	$(Q)echo "-> $@"
	$(Q)printf "#ifndef AM_VERSION_H\n#define AM_VERSION_H\n#define AM_VERSION \"" > $@
	$(Q)printf $(VERSION) >> $@
	$(Q)printf "; http://bwachter.lart.info/projects/aardmail/\"\n#endif\n" >> $@

dyn-conf.mk:
	$(Q)IBAARD="";if [ -d ibaard ]; then IBAARD=ibaard; echo "-> including local libaard";\
	else if [ -d ../ibaard ]; then IBAARD=../ibaard; echo "-> including local ../libaard";\
	fi; fi; if [ ! -z $IBAARD ]; then\
	printf "LIBS+=-L$$IBAARD\n";\
	printf "INCLUDES+=-I$$IBAARD/src\n";\
	printf "ALL=$$IBAARD/libibaard.a $(ALL)\n";\
	printf "CLEANDEPS=$$IBAARD-clean\n";\
	fi > $@

dyn-gmake.mk:
	$(Q)for i in 1; do \
	printf '$$(OBJDIR)/%%.o: $$(SRCDIR)/%%.c\n';\
	printf '\t$$(Q)echo "CC $$@"\n';\
	printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(CFLAGS) -c $$< -o $$@\n';\
	printf 'ifdef $$(STRIP)\n';\
	printf '\t$$(Q)$$(COMMENT) -$$(CROSS)$$(STRIP) $$@\n';\
	printf 'endif\n\n';\
	printf '%%.o: %%.c\n';\
	printf '\t$$(Q)echo "CC $$@"\n';\
	printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(CFLAGS) -c $$< -o $$@\n';\
	printf 'ifdef $$(STRIP)\n';\
	printf '\t$$(Q)$$(COMMENT) -$$(CROSS)$$(STRIP) $$@\n';\
	printf 'endif\n\n';\
	done > $@
	$(Q)for i in $(BD_BIN); do \
	printf "$$i: " ;\
	DEPS=`grep $$i targets | sed "s/\$$i//" | sed "s/\.exe//" | sed "s/\.c/\.o/g" | sed 's,src/,\$$(OBJDIR)/,g'`;\
	for j in $$DEPS; do printf "$$j "; done;\
	printf '\n\t$$(Q)echo "LD $$@"\n';\
	printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(LDFLAGS) -o $$@ $$^ $$(LIBS)\n\n';\
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
	printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(CFLAGS) -c $$< -o $$@\n';\
	printf '.ifdef $$(STRIP)\n';\
	printf '\t$$(Q)$$(COMMENT) -$$(CROSS)$$(STRIP) $$@\n';\
	printf '.endif\n\n';\
	done > $@
	$(Q)for i in $(BD_BIN); do \
	printf "$$i: " ;\
	DEPS=`grep $$i targets | sed "s/\$$i//" | sed "s/\.exe//" | sed "s/\.c/\.o/g" | sed 's,src/,\$$(OBJDIR)/,g'`;\
	for j in $$DEPS; do printf "$$j "; done;\
	printf '\n\t$$(Q)echo "LD $$@"\n';\
	printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(LDFLAGS) -o $$@ $$> $$(LIBS)\n\n';\
	done >> $@
	$(Q)for i in $(BD_LIB); do \
	printf "$$i:" ;\
	DEPS=`grep $$i targets | sed "s/\$$i//" | sed "s/\.c/\.o/g" | sed 's,src/,\$$(OBJDIR)/,g'`;\
	for j in $$DEPS; do printf "$$j "; done;\
	printf '\n\t$$(Q)echo "AR $$@"\n';\
	printf '\t$$(Q)$$(CROSS)$$(AR) $$(ARFLAGS) $$@ $$>\n\n';\
	printf '\t$$(Q)$$(CROSS)$$(RANLIB) $$@\n';\
	done >> $@

ibaard-clean: 
	$(Q)echo "-> cleaning up libaard"
	$(Q)make -C ibaard clean

../ibaard-clean: 
	$(Q)echo "-> cleaning up libaard"
	$(Q)make -C ../ibaard clean

clean: $(CLEANDEPS)
	$(Q)echo "-> cleaning up"
	$(Q)$(RM) $(ALL) *.exe $(OBJDIR)/*.{o,obj,lib} crammd5/*.{o,obj,lib} crammd5/*.o $(OBJDIR)/*.o $(SRCDIR)/version.h dyn-*.mk

distclean: clean
	$(Q)echo "-> cleaning up everything"
	$(Q)$(RM) -Rf ibaard

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
