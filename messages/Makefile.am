## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2001, 2002, 2003, 2004 Free Software Foundation, Inc.
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
##   Foundation, Inc. 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib\
 -I${top_builddir}/include/mailutils/gnu @INTLINCS@

SUBDIRS = testsuite

bin_PROGRAMS = messages
messages_SOURCES = messages.c
messages_LDADD =\
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/imap/libmu_imap.la\
 ../mailbox/pop/libmu_pop.la\
 ../mailbox/mh/libmu_mh.la\
 ../mailbox/maildir/libmu_maildir.la\
 ../mailbox/libmailbox.la\
 ../lib/libmailutils.la\
 @AUTHLIBS@ @MU_COMMON_LIBRARIES@
