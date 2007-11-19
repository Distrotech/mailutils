## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2000, 2001, 2002, 2003,
##   2004, 2005, 2007 Free Software Foundation, Inc.
##
##   GNU Mailutils is free software; you can redistribute it and/or
##   modify it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 3, or (at
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
EXTRA_DIST=errno.hin types.hin
errno.h: $(top_srcdir)/mailbox/errors errno.hin
	$(AWK) -f $(top_srcdir)/scripts/generr.awk $^ > $@

types.h: $(top_srcdir)/include/mailutils/types.hin Makefile
	sed 's/MU_OFF_TYPE/$(MU_OFF_TYPE)/' $(top_srcdir)/include/mailutils/types.hin > $@
DISTCLEANFILES = types.h

pkginclude_DATA = types.h
pkginclude_HEADERS = \
 address.h\
 argcv.h\
 assoc.h\
 attribute.h\
 auth.h\
 body.h\
 cfg.h\
 daemon.h\
 debug.h\
 envelope.h\
 errno.h\
 error.h\
 filter.h\
 folder.h\
 gocs.h\
 gsasl.h\
 guile.h\
 header.h\
 iterator.h\
 libargp.h\
 libcfg.h\
 libsieve.h\
 list.h\
 locker.h\
 mailbox.h\
 mailcap.h\
 mailer.h\
 mailutils.h\
 md5.h\
 message.h\
 mime.h\
 monitor.h\
 mu_auth.h\
 mutil.h\
 nls.h\
 nntp.h\
 observer.h\
 pam.h\
 parse822.h\
 pop3.h\
 property.h\
 radius.h\
 refcount.h\
 registrar.h\
 sha1.h\
 stream.h\
 syslog.h\
 sql.h\
 tls.h\
 url.h\
 version.h

nobase_pkginclude_HEADERS = \
  sys/nntp.h\
  sys/pop3.h
SUBDIRS = sys

