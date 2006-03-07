## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2000,2001,2002,2003,2004,2005 Free Software Foundation, Inc.
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

AUTOMAKE_OPTIONS = gnits 1.8.5 dist-bzip2 std-options
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = \
 include\
 intl\
 po\
 testsuite\
 mailbox\
 sql\
 auth\
 libproto\
 lib\
 libsieve\
 libmu_scm\
 doc\
 config\
 examples\
 frm\
 pop3d\
 imap4d\
 mail\
 sieve\
 scripts\
 guimb\
 messages\
 comsat\
 readmsg\
 mail.local\
 mail.remote\
 dotlock\
 mh\
 movemail\
 mimeview

EXTRA_DIST = COPYING.LESSER

alpha:
	$(MAKE) dist distdir=$(PACKAGE)-`date +"%Y%m%d"`

alphacheck:
	$(MAKE) distcheck distdir=$(PACKAGE)-`date +"%Y%m%d"`

rpm: dist
	rpm -ta --clean mailutils-$(VERSION).tar.gz

