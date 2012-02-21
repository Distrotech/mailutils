## This file is part of GNU Mailutils.
## Copyright (C) 2000-2005, 2007-2012 Free Software Foundation, Inc.
##
## GNU Mailutils is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as
## published by the Free Software Foundation; either version 3, or (at
## your option) any later version.
##
## GNU Mailutils is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.

BUILT_SOURCES=errno.h types.h
EXTRA_DIST=errno.hin types.hin
errno.h: $(top_srcdir)/libmailutils/diag/errors errno.hin
	$(AM_V_GEN)$(AWK) -f $(mu_aux_dir)/generr.awk $^ > $@

types.h: $(top_srcdir)/include/mailutils/types.hin Makefile
	$(AM_V_GEN)sed 's/_MU_OFF_TYPE_/$(MU_OFF_TYPE)/;s/_MU_DEFAULT_RECORD_/$(MU_DEFAULT_RECORD)/' $(top_srcdir)/include/mailutils/types.hin > $@
DISTCLEANFILES = types.h

pkginclude_HEADERS = \
 acl.h\
 address.h\
 alloc.h\
 argcv.h\
 assoc.h\
 attribute.h\
 auth.h\
 body.h\
 cctype.h\
 cfg.h\
 cidr.h\
 cstr.h\
 datetime.h\
 daemon.h\
 dbm.h\
 debug.h\
 diag.h\
 envelope.h\
 errno.h\
 error.h\
 filter.h\
 folder.h\
 gocs.h\
 gsasl.h\
 guile.h\
 header.h\
 imap.h\
 imapio.h\
 imaputil.h\
 io.h\
 iterator.h\
 kwd.h\
 ldap.h\
 libargp.h\
 libcfg.h\
 list.h\
 locker.h\
 log.h\
 mailbox.h\
 mailcap.h\
 mailer.h\
 mailutils.h\
 md5.h\
 message.h\
 mh.h\
 mime.h\
 monitor.h\
 msgset.h\
 mu_auth.h\
 util.h\
 nls.h\
 nntp.h\
 observer.h\
 opool.h\
 pam.h\
 parse822.h\
 pop3.h\
 prog.h\
 progmailer.h\
 property.h\
 python.h\
 radius.h\
 refcount.h\
 registrar.h\
 sha1.h\
 secret.h\
 server.h\
 sieve.h\
 smtp.h\
 sockaddr.h\
 stdstream.h\
 stream.h\
 syslog.h\
 sql.h\
 tls.h\
 types.h\
 url.h\
 vartab.h\
 version.h\
 wordsplit.h

if MU_COND_SUPPORT_CXX
  CPP_DIR = cpp
endif

SUBDIRS = sys $(CPP_DIR)

