AUTOMAKE_OPTIONS = gnu 1.6
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = include doc m4 lib MySql mailbox frm from pop3d imap4d \
          mail sieve scripts libmu_scm guimb messages comsat readmsg \
          mail.local mail.remote dotlock mh

EXTRA_DIST = mailutils.spec mailutils.spec.in README-alpha COPYING.FDL

release:
	$(MAKE) dist distdir=$(PACKAGE)-`date +"%Y%m%d"`

rpm: dist
	rpm -ta --clean mailutils-$(VERSION).tar.gz


## Override distcheck rule.
distcheck: dist
	$(am__remove_distdir)
	GZIP=$(GZIP_ENV) gunzip -c $(distdir).tar.gz | $(AMTAR) xf -
	chmod -R a-w $(distdir); chmod a+w $(distdir)
	chmod -R u+w $(distdir)/mail/testsuite/spool;
	chmod -R u+w $(distdir)/mail/testsuite/folder;
	mkdir $(distdir)/=build
	mkdir $(distdir)/=inst
	chmod a-w $(distdir)
	dc_install_base=`$(am__cd) $(distdir)/=inst && pwd` \
	 && cd $(distdir)/=build \
	 && ../configure --srcdir=.. --prefix=$$dc_install_base \
	$(DISTCHECK_CONFIGURE_FLAGS) \
	 && $(MAKE) $(AM_MAKEFLAGS) \
	 && $(MAKE) $(AM_MAKEFLAGS) dvi \
	 && $(MAKE) $(AM_MAKEFLAGS) check \
	 && $(MAKE) $(AM_MAKEFLAGS) install \
	 && $(MAKE) $(AM_MAKEFLAGS) installcheck \
	 && $(MAKE) $(AM_MAKEFLAGS) uninstall \
	 && $(MAKE) $(AM_MAKEFLAGS) dist-gzip \
	 && rm -f $(distdir).tar.gz \
	 && $(MAKE) $(AM_MAKEFLAGS) distcleancheck
	$(am__remove_distdir)
	@echo "$(distdir).tar.gz is ready for distribution" | \
	 sed 'h;s/./=/g;p;x;p;x'
 

