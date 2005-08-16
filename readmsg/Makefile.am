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
##   Foundation, Inc. 51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib\
 -I${top_builddir}/include/mailutils/gnu @INTLINCS@

SUBDIRS = testsuite

bin_PROGRAMS = readmsg
readmsg_SOURCES = readmsg.c msglist.c readmsg.h

readmsg_LDADD =\
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/imap/libmu_imap.la\
 ../mailbox/pop/libmu_pop.la\
 ../mailbox/nntp/libmu_nntp.la\
 ../mailbox/mh/libmu_mh.la\
 ../mailbox/maildir/libmu_maildir.la\
 ../mailbox/libmailutils.la\
 ../lib/libmuaux.la\
 @AUTHLIBS@ @MU_COMMON_LIBRARIES@
