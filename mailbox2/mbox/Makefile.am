## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2001, 2002 Free Software Foundation, Inc.
##
##   GNU Mailtuils is free software; you can redistribute it and/or
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

AUTOMAKE_OPTIONS = ../lib/ansi2knr

#INCLUDES = -I${top_srcdir}/include
INCLUDES = -I${top_srcdir}/mailbox2/include

EXTRA_DIST = valid.h

lib_LTLIBRARIES = libmbox.la

libmbox_la_SOURCES = \
 mbox_attribute.c    \
 mbox_blines.c       \
 mbox_bsize.c        \
 mbox_bstream.c      \
 mbox_carrier.c      \
 mbox_cb.c           \
 mbox_close.c        \
 mbox_create.c       \
 mbox_debug.c        \
 mbox_destroy.c      \
 mbox_expunge.c      \
 mbox_hcache.c       \
 mbox_hlines.c       \
 mbox_hsize.c        \
 mbox_hstream.c      \
 mbox_newmail.c      \
 mbox_open.c         \
 mbox_scan.c         \
 mbox_separator.c    \
 mbox_size.c         \
 mbox_stream.c       \
 mbox_uid.c          \
 mbox_uidnext.c      \
 mbox_uidvalidity.c
