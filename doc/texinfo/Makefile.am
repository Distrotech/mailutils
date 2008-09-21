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

RENDITION_TEXI=rendition.texi macros.texi

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
 $(RENDITION_TEXI)\
 $(INCFILES)

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

# The rendering level is one of PUBLISH, DISTRIB or PROOF.
# Just call `make RENDITION=PROOF [target]' if you want PROOF rendition.

MAKEINFOFLAGS=-D$(RENDITION)
GENDOCS=gendocs.sh
TEXI2DVI=texi2dvi -t '@set $(RENDITION)' -E

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


# Checks
check-format:
	@if test -n "`cat $(info_TEXINFOS) $(mailutils_TEXINFOS) |\
             tr -d -c '\t'`"; then \
		echo "Sources contain tabs; run make untabify"; \
		false; \
	fi

check-refs:
	@for file in $(info_TEXINFOS) $(dico_TEXINFOS); \
	do \
	  sed -e = $$file | \
           sed -n 'N;/@FIXME-.*ref/{s/\(^[0-9][0-9]*\).*@FIXME-.*ref{\([^}]*\)}.*/'$$file':\1: \2/gp}'; \
	done > $@-t; \
	if [ -s $@-t ]; then \
	  echo "Unresolved cross-references:"; \
	  cat $@-t;\
	  rm $@-t; \
	else \
	  rm -f $@-t; \
	fi

check-fixmes:
	@for file in $(info_TEXINFOS); \
	do \
	  sed -e = $$file | \
           sed -n 'N;/@FIXME{/{s/\(^[0-9][0-9]*\).*@FIXME{\([^}]*\).*/'$$file':\1: \2/gp}'; \
	done > $@-t; \
	if [ -s $@-t ]; then \
	  echo "Unresolved FIXMEs:"; \
	  cat $@-t; \
	  rm $@-t; \
	  false; \
	else \
          rm -f $@-t; \
	fi

check-unrevised:
	@grep -Hn @UNREVISED $(info_TEXINFOS) $(dico_TEXINFOS) > $@-t; \
	if [ -s $@-t ]; then \
	  echo "Unrevised nodes:"; \
	  cat $@-t; \
	  rm $@-t; \
	  false;\
	else \
          rm $@-t; \
	fi

all-check-docs: check-format check-refs check-fixmes check-unrevised

check-docs: 
	$(MAKE) -k all-check-docs
