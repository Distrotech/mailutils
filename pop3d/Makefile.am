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
##   Foundation, Inc.
##   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I${top_builddir}/include/mailutils/gnu -I$(top_srcdir)/include @INTLINCS@

sbin_PROGRAMS = pop3d
bin_PROGRAMS = @POPAUTH@
EXTRA_PROGRAMS = popauth
SUBDIRS = testsuite

pop3d_SOURCES = apop.c auth.c capa.c dele.c extra.c pop3d.c pop3d.h \
 list.c lock.c noop.c quit.c retr.c rset.c stat.c stls.c signal.c top.c \
 uidl.c user.c logindelay.c expire.c

pop3d_LDADD = \
 @AUTHLIBS@ \
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/mh/libmu_mh.la\
 ../mailbox/maildir/libmu_maildir.la\
 ../mailbox/libmailbox.la\
 ../lib/libmailutils.la\
 @MU_COMMON_LIBRARIES@

popauth_SOURCES = popauth.c
popauth_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.la @MU_COMMON_LIBRARIES@
pop3d_DEPENDENCIES = \
 @AUTHLIBS_DEPENDENCY@ \
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/mh/libmu_mh.la\
 ../mailbox/maildir/libmu_maildir.la\
 ../mailbox/libmailbox.la\
 ../lib/libmailutils.la
