common-clean:
	$(Q)$(RM) $(BD_OBJ)/*.{o,obj,lib} dyn-*.mk *.{a,exe,lib,tds} *.so* $(BD_BINDIR)/* $(BD_LIBDIR)/*

dyn-conf.mk: targets build.mk $(LOCAL_CONF)
	$(Q)echo "ALL=" > $@
	$(Q)if [ -n "$(LOCAL_CONF)" ]; then echo "$(MK_INCLUDE) $(LOCAL_CONF)"; fi >> $@
	$(Q)for i in $(BD_LIB); do \
	  if [ "x$(STATIC_LIBRARY)" = "x1" ]; then printf "ALL+=$(BD_LIBDIR)lib$$i.a\n"; fi ;\
	  if [ "x$(DYNAMIC_LIBRARY)" = "x1" ]; then printf "ALL+=$(BD_LIBDIR)lib$$i.so.$(MAJOR).$(MINOR).$(RELEASE)\n"; fi \
	done >> $@
	$(Q)echo "ALL+=`echo $(BD_BIN)|sed 's,^,$(BD_BINDIR),'|sed 's, , $(BD_BINDIR),g'`" >> $@
	$(Q)if [ -f doc/Doxyfile ]; then echo "ALL+=doc"; fi >> $@

# Generic rules for building binaries
dyn-binary-targets.mk: targets build.mk system.mk
	$(Q)echo > $@; mkdir -p $(BD_BINDIR); for i in $(BD_BIN); do \
	  echo -n "DEP LD $$i... " >&2;\
	  printf "$(BD_BINDIR)$$i: " ;\
	DEPS=`grep $$i targets | sed "s/\$$i//" | sed "s/\.exe//" | sed "s/\.c/\.o/g" | sed 's,src/,\$$(BD_OBJ)/,g'`;\
	for j in $$DEPS; do echo -n "$$j " >&2; printf "$$j "; done;\
	printf '\n\t$$(Q)echo "LD $$@"\n';\
	printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(LDFLAGS) $$(INCLUDES) -o $$@ $(MK_ALL) $$(LIBS)\n\n';\
	  echo "" >&2 ;\
	done 2>&1 >> $@

# Generic rules for building libraries
dyn-library-targets.mk: targets build.mk system.mk
	$(Q)echo > $@; mkdir -p $(BD_LIBDIR); for i in $(BD_LIB); do \
	  _LIB=lib$$i ;\
	  echo -n "DEP LIB $$i... " >&2;\
	  DEPS=`grep $$i.lib targets | sed "s/\$$i//" | sed "s/\.lib//" | sed "s/\.c/\.o/g" | sed 's,src/,\$$(BD_OBJ)/,g'`;\
	  printf "$(BD_LIBDIR)$$_LIB.a:" ;\
	for j in $$DEPS; do printf "$$j "; done;\
	printf '\n\t$$(Q)echo "AR $$@"\n';\
	printf '\t$$(Q)$$(CROSS)$$(AR) $$(ARFLAGS) $$@ $(MK_ALL)\n';\
	printf '\t$$(Q)$$(CROSS)$$(RANLIB) $$@\n';\
	printf '\n';\
	  printf "$(BD_LIBDIR)$$_LIB.so.$(MAJOR).$(MINOR).$(RELEASE):" ;\
	for j in $$DEPS; do echo -n "$$j " >&2; printf "$$j "; done;\
	printf '\n\t$$(Q)echo "SO $$@"\n';\
	printf "\t\$$(Q)\$$(DIET) \$$(CROSS)\$$(CC) -shared -Wl,-soname,$$_LIB.so.$(MAJOR) \$$(INCLUDES) -o \$$@ $(MK_ALL)\n";\
	printf "\t\$$(Q)ln -s \$$@ $(BD_LIBDIR)$$_LIB.so\n";\
	printf "\t\$$(Q)ln -s \$$@ $(BD_LIBDIR)$$_LIB.so.$(MAJOR)\n";\
	printf "\t\$$(Q)ln -s \$$@ $(BD_LIBDIR)$$_LIB.so.$(MAJOR).$(MINOR)\n";\
	printf '\n';\
	  echo "" >&2 ;\
	done 2>&1 >> $@

# Generic make rules for GNU make
dyn-gmake.mk: dyn-binary-targets.mk dyn-library-targets.mk dyn-tests.mk
	$(Q)for i in 1; do \
	  printf '%%.o: %%.c\n';\
	printf '\t$$(Q)echo "CC $$@"\n';\
	printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(CFLAGS) $$(INCLUDES) -c $$< -o $$@\n';\
	  printf 'include dyn-binary-targets.mk dyn-library-targets.mk dyn-tests.mk' ;\
	done > $@

# Generic make rules for BSD make
dyn-bsdmake.mk: dyn-binary-targets.mk dyn-library-targets.mk dyn-tests.mk
	$(Q)for i in 1; do \
	  printf '.c.o:\n';\
	printf '\t$$(Q)echo "CC $$@"\n';\
	printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(CFLAGS) $$(INCLUDES) -c $$< -o $$@\n';\
	  printf 'include dyn-binary-targets.mk dyn-library-targets.mk dyn-tests.mk' ;\
	done > $@

# Build tests in tests/ directory
# This assumes that all tests will be run from one main binary
dyn-tests.mk: build.mk system.mk
	$(Q)printf "test: " > $@
	$(Q)for i in tests/*.c; do \
	  DEP=`echo $$i | sed "s/\.c/\.o/g"` ;\
	  printf " $$DEP" ;\
	done >> $@
	$(Q)for i in 1; do \
	  printf '\n\t$$(Q)echo "LD $$@"\n' ;\
	  printf '\t$$(Q)$$(DIET) $$(CROSS)$$(CC) $$(LDFLAGS) $$(INCLUDES) -o $$@ $(MK_ALL) $$(LIBS) -lcheck ' ;\
	  printf "`echo $(BD_LIB)|awk '{for (i=1;i<=NF;i++) printf " -l"$$i}'`\n" ;\
	  printf '\t$$(Q)rm -Rf test-run && mkdir -p test-run && ./$$@\n\n' ;\
	done >> $@
