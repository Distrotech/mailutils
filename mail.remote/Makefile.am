# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/include @INTLINCS@

SUBDIRS = testsuite
PROGRAMS_SMTP = mail.remote
EXTRA_PROGRAMS = mail.remote
libexec_PROGRAMS = @BUILD_SMTP_PROGRAMS@

mail_remote_SOURCES = mail.remote.c 

mail_remote_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.la @LTLIBINTL@
