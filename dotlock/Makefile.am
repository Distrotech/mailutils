# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/include -I$(top_srcdir)/libmu_scm 

bin_PROGRAMS = dotlock
#dotlock_SOURCES = dotlock.c

dotlock_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.la 

install-exec-hook:
	for i in $(bin_PROGRAMS); do\
		chown root:mail $(DESTDIR)$(bindir)/$$i;\
		chmod 2755 $(DESTDIR)$(bindir)/$$i;\
	done
