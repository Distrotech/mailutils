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
##   Foundation, Inc.
##   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

AUTOMAKE_OPTIONS = gnits 1.8.5 dist-bzip2 std-options
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = include intl po lib testsuite mailbox libsieve doc \
          sql auth examples frm pop3d imap4d \
          mail sieve scripts libmu_scm guimb messages comsat readmsg \
          mail.local mail.remote dotlock mh movemail mimeview

EXTRA_DIST = COPYING.LESSER @MU_EXTRA_DIST@

release:
	$(MAKE) dist distdir=$(PACKAGE)-`date +"%Y%m%d"`

rpm: dist
	rpm -ta --clean mailutils-$(VERSION).tar.gz

