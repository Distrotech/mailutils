# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

info_TEXINFOS = mailutils.texi
mailutils_TEXINFOS = \
 address.texi \
 attribute.texi \
 auth.texi \
 body.texi \
 c-api.texi \
 encoding.texi \
 envelope.texi \
 http.c.texi \
 address.c.texi \
 url-parse.c.texi \
 framework.texi \
 folder.texi \
 headers.texi \
 imap4.texi \
 iterator.texi \
 locker.texi \
 mailbox.texi \
 mailer.texi \
 maildir.texi \
 mbox.texi \
 message.texi \
 mh.texi \
 nntp.texi \
 parse822.texi \
 pop3.texi \
 programs.texi \
 sendmail.texi \
 sfrom.c.texi \
 smtp.texi \
 stream.texi \
 url.texi

address.c.texi: ${top_srcdir}/examples/addr.c
	sed '1,/.*\*\//d;s/\([{}]\)/@\1/g' $< > $@

url-parse.c.texi: ${top_srcdir}/examples/url-parse.c
	sed '1,/.*\*\//d;s/\([{}]\)/@\1/g' $< > $@

sfrom.c.texi: ${top_srcdir}/examples/sfrom.c
	sed '1,/.*\*\//d;s/\([{}]\)/@\1/g' $< > $@

http.c.texi: ${top_srcdir}/examples/http.c
	sed '1,/.*\*\//d;s/\([{}]\)/@\1/g' $< > $@

DISTCLEANFILES=*.pgs
