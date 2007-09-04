## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2001, 2002, 2003, 2004, 2006, 
##   2007 Free Software Foundation, Inc.
##
##   GNU Mailutils is free software; you can redistribute it and/or
##   modify it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 3, or (at
##   your option) any later version.
##
##   This program is distributed in the hope that it will be useful, but
##   WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##   General Public License for more details.
##
##   You should have received a copy of the GNU General Public License
##   along with this program; if not, write to the Free Software
##   Foundation, Inc. 51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

info_TEXINFOS = mailutils.texi 

INCFILES = \
 addr.inc\
 http.inc\
 mailcap.inc\
 numaddr.inc\
 sfrom.inc\
 url-parse.inc

mailutils_TEXINFOS = \
 address.texi\
 attribute.texi\
 auth.texi\
 body.texi\
 c-api.texi\
 encoding.texi\
 envelope.texi\
 fdl.texi\
 folder.texi\
 framework.texi\
 headers.texi\
 imap4.texi\
 iterator.texi\
 libmu_scm.texi\
 libmuauth.texi\
 libsieve.texi\
 locker.texi\
 mailbox.texi\
 mailcap.texi\
 maildir.texi\
 mailer.texi\
 mbox.texi\
 message.texi\
 mh.texi\
 mu-mh.texi\
 mu_address.texi\
 mu_body.texi\
 mu_logger.texi\
 mu_mailbox.texi\
 mu_message.texi\
 mu_mime.texi\
 mu_scm.texi\
 nntp.texi\
 parse822.texi\
 pop3.texi\
 programs.texi\
 sendmail.texi\
 sieve.texi\
 smtp.texi\
 stream.texi\
 url.texi\
 $(INCFILES)

muint_TEXINFOS = \
 muint.texi\
 mom.texi

## Fake configure into including srcdir to VPATH:
s=${srcdir}:${top_srcdir}/examples/
VPATH = $(s)

SUFFIXES=.c .inc 
.c.inc:
	sed -f $(top_srcdir)/scripts/texify.sed $< > $@

DISTCLEANFILES=*.pgs *.kys *.vrs
MAINTAINERCLEANFILES=$(INCFILES)
clean-local:
	rm -rf manual

GENDOCS=gendocs.sh
TEXI2DVI=texi2dvi
MAKEINFOFLAGS=

# Make sure you set TEXINPUTS.
# TEXINPUTS=/usr/share/texmf/pdftex/plain/misc/ is ok for most distributions

TEXINPUTS=$(srcdir):$(top_srcdir)/scripts:$$TEXINPUTS

manual:
	 TEXINPUTS=$(srcdir):$(top_srcdir)/scripts:$(TEXINPUTS) \
	 MAKEINFO="$(MAKEINFO) $(MAKEINFOFLAGS)" \
	 TEXI2DVI="$(TEXI2DVI) -t @finalout" \
	$(GENDOCS) $(PACKAGE) '$(PACKAGE_NAME) manual'


EXTRA_DIST=gendocs_template mastermenu.el untabify.el

master-menu:
	emacs -batch -l mastermenu.el -f make-master-menu $(info_TEXINFOS)

untabify:
	emacs -batch -l untabify.el $(info_TEXINFOS) $(mailutils_TEXINFOS)

final: untabify master-menu
