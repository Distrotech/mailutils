## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2002 Free Software Foundation, Inc.
##
##   GNU Mailutils is free software; you can redistribute it and/or
##   modify it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 2, or (at
##   your option) any later version.
##
##   This program is distributed in the hope that it will be useful, but
##   WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##   General Public License for more details.
##
##   You should have received a copy of the GNU General Public License
##   along with this program; if not, write to the Free Software
##   Foundation, Inc.  51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I${top_builddir}/include/mailutils/gnu -I$(top_srcdir)/mailbox -I$(top_srcdir)/include @GUILE_INCLUDES@ @INTLINCS@
SUBDIRS = testsuite

libexec_PROGRAMS = mail.local
mail_local_SOURCES = main.c mailquota.c script.c mail.local.h

mail_local_LDADD = \
 @LIBMU_SCM@ @GUILE_LIBS@\
 @LIBMU_SCM_DEPS@\
 @AUTHLIBS@\
 ../mailbox/mbox/libmu_mbox.la \
 ../mailbox/libmailutils.la \
 ../libsieve/libsieve.la\
 ../lib/libmuaux.la @MU_COMMON_LIBRARIES@

install-exec-hook:
	for i in $(libexec_PROGRAMS); do\
		chown root:mail $(DESTDIR)$(libexecdir)/$$i;\
		chmod 4755 $(DESTDIR)$(libexecdir)/$$i;\
	done
