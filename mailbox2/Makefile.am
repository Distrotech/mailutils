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

SUBDIRS = include pop3 mbox


lib_LTLIBRARIES = libmailbox.la

libmailbox_la_SOURCES = \
 address.c    \
 attribute.c  \
 authority.c  \
 bstream.c    \
 dattribute.c \
 debug.c      \
 dotlock.c    \
 envelope.c   \
 fdstream.c   \
 folder.c     \
 fstream.c    \
 header.c     \
 iterator.c   \
 list.c       \
 lockfile.c   \
 mailbox.c    \
 md5-rsa.c    \
 memstream.c  \
 message.c    \
 mapstream.c  \
 mutil.c      \
 observable.c \
 observer.c   \
 parse822.c   \
 pticket.c    \
 refcount.c   \
 sdebug.c     \
 stream.c     \
 tcpstream.c  \
 ticket.c
