# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

AUTOMAKE_OPTIONS = gnu 1.6 readme-alpha
ACLOCAL_AMFLAGS = -I m4

# directories depending on mh support
MH_DIRS = mh
MH_BUILD_DIRS = @MH_BUILD_DIRS@

# directories depending on the smtp protocol
SMTP_DIRS = mail.remote
SMTP_BUILD_DIRS = @SMTP_BUILD_DIRS@

SUBDIRS = include m4 lib testsuite mailbox examples doc \
          auth frm from pop3d imap4d \
          mail sieve scripts libmu_scm guimb messages comsat readmsg \
          mail.local $(SMTP_BUILD_DIRS) dotlock $(MH_DIRS)

EXTRA_DIST = mailutils.spec mailutils.spec.in COPYING.FDL

release:
	$(MAKE) dist distdir=$(PACKAGE)-`date +"%Y%m%d"`

rpm: dist
	rpm -ta --clean mailutils-$(VERSION).tar.gz


