AUTOMAKE_OPTIONS = gnu 1.6 
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = include m4 lib testsuite mailbox examples doc \
          auth frm from pop3d imap4d \
          mail sieve scripts libmu_scm guimb messages comsat readmsg \
          mail.local mail.remote dotlock mh

EXTRA_DIST = mailutils.spec mailutils.spec.in COPYING.FDL README-alpha

release:
	$(MAKE) dist distdir=$(PACKAGE)-`date +"%Y%m%d"`

rpm: dist
	rpm -ta --clean mailutils-$(VERSION).tar.gz


