# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/mailbox -I$(top_srcdir)/include -I$(top_srcdir)/libmu_scm @GUILE_INCLUDES@ @INTLINCS@
SUBDIRS = testsuite

libexec_PROGRAMS = mail.local
mail_local_SOURCES = main.c mailquota.c script.c mail.local.h

mail_local_LDADD = @LIBMU_SCM@ @GUILE_LIBS@ ../mailbox/libmailbox.la \
 @AUTHLIBS@ ../libsieve/libsieve.la ../lib/libmailutils.la @LTLIBINTL@
 

install-exec-hook:
	for i in $(libexec_PROGRAMS); do\
		chown root:mail $(DESTDIR)$(libexecdir)/$$i;\
		chmod 4755 $(DESTDIR)$(libexecdir)/$$i;\
	done
