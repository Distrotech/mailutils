# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

info_TEXINFOS = mailutils.texi

INCFILES=addr.inc http.inc numaddr.inc sfrom.inc url-parse.inc 

mailutils_TEXINFOS = \
 address.texi \
 attribute.texi \
 auth.texi \
 body.texi \
 c-api.texi \
 encoding.texi \
 envelope.texi \
 framework.texi \
 folder.texi \
 headers.texi \
 imap4.texi \
 iterator.texi \
 libmuauth.texi \
 libsieve.texi \
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
 smtp.texi \
 stream.texi \
 url.texi \
 $(INCFILES)

## Fake configure into including srcdir to VPATH:
s=${srcdir}:${top_srcdir}/examples/
VPATH = $(s)

SUFFIXES=.c .inc
.c.inc:
	sed '1{s/\/\*/@comment  /;b};2,/.*\*\//{s/\*\///;s/^/@comment/;b};s/\([{}]\)/@\1/g' $< > $@

DISTCLEANFILES=*.pgs 
MAINTAINERCLEANFILES=$(INCFILES)
