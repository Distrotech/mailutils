AUTOMAKE_OPTIONS = gnu 1.4
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = include doc m4 lib MySql argp mailbox frm from pop3d imap4d mail sieve \
	scripts libmu_scm guimb messages

EXTRA_DIST = mailutils.spec mailutils.spec.in README-alpha COPYING.FDL

release:
	$(MAKE) dist distdir=$(PACKAGE)-`date +"%Y%m%d"`

rpm: dist
	rpm -ta --clean mailutils-$(VERSION).tar.gz
