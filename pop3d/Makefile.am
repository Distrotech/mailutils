# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/include @INTLINCS@

sbin_PROGRAMS = pop3d
bin_PROGRAMS = @POPAUTH@
EXTRA_PROGRAMS = popauth
SUBDIRS = testsuite

pop3d_SOURCES = apop.c auth.c capa.c dele.c extra.c pop3d.c pop3d.h \
 list.c lock.c noop.c quit.c retr.c rset.c stat.c signal.c top.c uidl.c \
 user.c 

pop3d_LDADD = ../mailbox/libmailbox.la @AUTHLIBS@ ../lib/libmailutils.la @LTLIBINTL@

popauth_SOURCES = popauth.c
popauth_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.la @LTLIBINTL@
