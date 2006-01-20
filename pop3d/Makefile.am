## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 1999, 2000, 2001, 2002, 2003, 2005 Free Software Foundation, Inc.
##
##   GNU Mailutils is free software; you can redistribute it and/or
##   modify it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 2, or (at
##   your option) any later version.
##
##   GNU Mailutils is distributed in the hope that it will be useful, but
##   WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##   General Public License for more details.
##
##   You should have received a copy of the GNU General Public License
##   along with GNU Mailutils; if not, write to the Free Software
##   Foundation, Inc.  51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

INCLUDES = @MU_COMMON_INCLUDES@ @INTLINCS@

sbin_PROGRAMS = pop3d
bin_PROGRAMS = @POPAUTH@
EXTRA_PROGRAMS = popauth
SUBDIRS = testsuite

pop3d_SOURCES = apop.c auth.c capa.c dele.c extra.c pop3d.c pop3d.h \
 list.c lock.c noop.c quit.c retr.c rset.c stat.c stls.c signal.c top.c \
 uidl.c user.c logindelay.c expire.c

pop3d_LDADD = \
 ../lib/libmuaux.la\
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/mh/libmu_mh.la\
 ../mailbox/maildir/libmu_maildir.la\
 @MU_AUTHLIBS@ \
 ../mailbox/libmailutils.la\
 @MU_COMMON_LIBRARIES@

popauth_SOURCES = popauth.c
popauth_LDADD = ../lib/libmuaux.la ../mailbox/libmailutils.la @MU_COMMON_LIBRARIES@
pop3d_DEPENDENCIES = \
 @MU_AUTHLIBS_DEPENDENCY@ \
 ../lib/libmuaux.la \
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/mh/libmu_mh.la\
 ../mailbox/maildir/libmu_maildir.la\
 ../mailbox/libmailutils.la

CFLOW_FLAGS=-i^s --brief -v
pop3d_CFLOW_INPUT=$(pop3d_OBJECTS:.$(OBJEXT)=.c)
pop3d.cflow: $(pop3d_CFLOW_INPUT) Makefile
	cflow -o$@ $(CFLOW_FLAGS) $(DEFS) \
		$(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
		$(CPPFLAGS) \
	$(pop3d_CFLOW_INPUT)
