## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2000, 2001, 2002, 2003,
##   2004, 2005 Free Software Foundation, Inc.
##
##   GNU Mailutils is free software; you can redistribute it and/or
##   modify it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 2, or (at
##   your option) any later version.
##
##   GNU Mailutils is distributed in the hope that it will be useful, but
##   WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##   General Public License for more details.
##
##   You should have received a copy of the GNU General Public License
##   along with GNU Mailutils; if not, write to the Free Software
##   Foundation, Inc. 51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

BUILT_SOURCES=errno.h
EXTRA_DIST=errno.hin
errno.h: $(top_srcdir)/mailbox/errors errno.hin
	$(AWK) -f $(top_srcdir)/scripts/generr.awk $^ > $@

pkginclude_HEADERS = \
 address.h \
 argcv.h \
 argp.h \
 attribute.h \
 auth.h \
 body.h \
 daemon.h \
 debug.h \
 envelope.h \
 errno.h \
 error.h \
 filter.h \
 folder.h \
 gsasl.h \
 guile.h \
 header.h \
 iterator.h \
 libsieve.h \
 list.h \
 locker.h \
 mailbox.h \
 mailcap.h \
 mailer.h \
 mailutils.h \
 message.h \
 mime.h \
 monitor.h \
 mu_auth.h \
 mutil.h \
 nls.h \
 nntp.h \
 observer.h \
 parse822.h \
 pop3.h \
 property.h \
 refcount.h \
 registrar.h \
 stream.h \
 sql.h \
 tls.h \
 types.h \
 url.h 

nobase_pkginclude_HEADERS = \
  sys/nntp.h \
  sys/pop3.h
SUBDIRS = gnu sys

