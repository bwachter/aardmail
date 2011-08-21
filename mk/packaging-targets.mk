$(NAME).spec: CHANGES
	$(Q)echo "-> $@"
	$(Q)sed -i "s/Version:.*/Version: $(VERSIONNR)/" $@

maintainer-deb:
	$(MAKE) deb DPKGFLAGS=-ap

deb: dist
	$(Q)pwd|sed 's,.*/,,'|grep -q '-'; \
	  if [ $$? -ne 0 ]; then \
	rm -Rf $(VERSION) && \
	tar xf $(VERSION).tar.gz && \
	cd $(VERSION) ;\
	  fi ;\
	  if [ -z "$$DPKGFLAGS" ]; then DPKGFLAGS="-us -uc"; fi ;\
	  ./debchangelog && \
	  dpkg-buildpackage -rfakeroot $$DPKGFLAGS

rpm: dist
	$(Q)if [ -d .git ] && [ ! -z "`git status -s $(NAME).spec`" ]; then \
	  echo "$(NAME).spec updated, but not comitted. Can't continue."; \
	  false; \
	fi
	$(Q)if [ -f $(VERSION).tar.gz ]; then rpmbuild -ta $(VERSION).tar.gz ;\
	  elif [ -f ../$(VERSION).tar.gz ]; then rpmbuild -ta ../$(VERSION).tar.gz ;\
	  else echo "$(VERSION).tar.gz or ../$(VERSION).tar.gz not found, unable to build RPM" ;\
	fi

