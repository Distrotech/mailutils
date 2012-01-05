# GNU Mailutils -- a suite of utilities for electronic mail
# Copyright (C) 2010-2012 Free Software Foundation, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General
# Public License along with this library.  If not, see
# <http://www.gnu.org/licenses/>.

noinst_LTLIBRARIES = libbase.la

libbase_la_SOURCES = \
 alloc.c\
 amd.c\
 argcvfree.c\
 argcvjoin.c\
 argcvrem.c\
 assoc.c\
 filesafety.c\
 daemon.c\
 fdwait.c\
 fgetpwent.c\
 filename.c\
 freeitem.c\
 fscompose.c\
 getcwd.c\
 getmaxfd.c\
 getpass.c\
 hostname.c\
 iterator.c\
 kwd.c\
 lcall.c\
 locale.c\
 locker.c\
 mailcap.c\
 md5.c\
 monitor.c\
 msgid.c\
 munre.c\
 mutil.c\
 nls.c\
 nullrec.c\
 observer.c\
 onexit.c\
 opool.c\
 parsedate.c\
 permstr.c\
 registrar.c\
 refcount.c\
 rfc2047.c\
 schemeauto.c\
 sha1.c\
 secret.c\
 spawnvp.c\
 symlink.c\
 tempfile.c\
 ticket.c\
 tilde.c\
 userprivs.c\
 usremail.c\
 version.c\
 wicket.c

localedir = $(datadir)/locale
AM_CPPFLAGS = \
 -DSYSCONFDIR=\"$(sysconfdir)\"\
 -DSITE_VIRTUAL_PWDDIR=\"@SITE_VIRTUAL_PWDDIR@\"\
 -DLOCALEDIR=\"$(localedir)\"

INCLUDES = @MU_LIB_COMMON_INCLUDES@ -I/libmailutils

YLWRAP = $(SHELL) $(mu_aux_dir)/gylwrap
AM_YFLAGS=-vt
AM_LFLAGS=-dvp

EXTRA_DIST = parsedate.y

BUILT_SOURCES = parsedate.c

parsedate.c: $(srcdir)/parsedate.y
	$(YLWRAP) "$(YACC) $(AM_YFLAGS)" $< \
	          y.tab.c parsedate.c y.output parsedate.y.output \
                  -- -yy pd_yy


