## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2003, 2005, 2006 Free Software Foundation, Inc.
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

INCLUDES = @MU_COMMON_INCLUDES@ -I${top_srcdir}/mailbox/include @INTLINCS@

lib_LTLIBRARIES = libmu_imap.la
libmu_imap_la_LDFLAGS=-version-info @VI_CURRENT@:@VI_REVISION@:@VI_AGE@
libmu_imap_la_LIBADD = ../libmailutils.la @INTLLIBS@ 

libmu_imap_la_SOURCES = \
 folder.c\
 mbox.c\
 url.c

