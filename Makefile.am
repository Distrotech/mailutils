# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

AUTOMAKE_OPTIONS = gnu 1.7.1 readme-alpha std-options
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = include m4 lib testsuite mailbox libsieve examples doc \
          auth frm from pop3d imap4d \
          mail sieve scripts libmu_scm guimb messages comsat readmsg \
          mail.local mail.remote dotlock mh 

EXTRA_DIST = mailutils.spec mailutils.spec.in COPYING.FDL

release:
	$(MAKE) dist distdir=$(PACKAGE)-`date +"%Y%m%d"`

rpm: dist
	rpm -ta --clean mailutils-$(VERSION).tar.gz


