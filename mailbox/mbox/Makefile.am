## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2003 Free Software Foundation, Inc.
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
##   Foundation, Inc.
##   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/mailbox \
 -I${top_srcdir}/mailbox/include -I${top_builddir}/include/mailutils/gnu \
 @INTLINCS@

lib_LTLIBRARIES = libmu_mbox.la

libmu_mbox_la_SOURCES = \
 folder.c\
 mbox.c\
 mboxscan.c\
 mbox0.h\
 url.c

