AUTOMAKE_OPTIONS = gnu 1.4
ACLOCAL_AMFLAGS = -I m4

# imap4d
SUBDIRS = include doc m4 lib mailbox frm from pop3d imap4d mail

EXTRA_DIST = mailutils.spec mailutils.spec.in README-alpha COPYING.FDL

release:
	$(MAKE) dist distdir=$(PACKAGE)-`date +"%Y%m%d"`

rpm: dist
	rpm -ta --clean mailutils-$(VERSION).tar.gz
