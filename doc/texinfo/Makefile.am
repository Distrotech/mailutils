## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2001, 2002 Free Software Foundation, Inc.
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
 fdl.texi \
 framework.texi \
 folder.texi \
 headers.texi \
 imap4.texi \
 iterator.texi \
 libmuauth.texi \
 libmu_scm.texi \
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

## Web documentation
BASE_URL="http://www.gnu.org/software/mailutils/manual/"
TEXI2HTML=${SHELL} `cd $(top_srcdir); pwd`/scripts/missing --run texi2html
DVIPS=${SHELL} `cd $(top_srcdir); pwd`/scripts/missing --run dvips
INFO=${SHELL} `cd $(top_srcdir); pwd`/scripts/missing --run info
EXTRA_DIST = index.html.in

html: html_mono html_node html_chapter

html_node:
	texi2html="$(TEXI2HTML)";\
	if ! test -d html_node; then mkdir html_node; fi; \
	cd html_node; \
	$$texi2html -menu -split_node ../mailutils.texi

html_chapter:
	texi2html="$(TEXI2HTML)";\
	if ! test -d html_chapter; then mkdir html_chapter; fi; \
	cd html_chapter; \
	$$texi2html -menu -split_chapter ../mailutils.texi

html_mono:
	texi2html="$(TEXI2HTML)";\
	if ! test -d html_mono; then mkdir html_mono; fi; \
	cd html_mono; \
	$$texi2html -menu -monolithic ../mailutils.texi

ps: mailutils.ps

mailutils.ps: mailutils.dvi
	$(DVIPS) -omailutils.ps mailutils.dvi

text: mailutils.text

mailutils.text: mailutils.info
	$(INFO) --node=Top --subnodes --out mailutils.text -f mailutils.info

mailutils.info.tar.gz:
	tar cfz mailutils.info.tar.gz mailutils.info*

mailutils.dvi.gz: mailutils.dvi
	gzip -c mailutils.dvi > mailutils.dvi.gz

mailutils.ps.gz: mailutils.ps
	gzip -c mailutils.ps > mailutils.ps.gz

mailutils.texi.tar.gz:
	tar cfz mailutils.texi.tar.gz *.texi

WEB_HTML=\
 html_mono\
 html_node\
 html_chapter

WEB_BIN=\
 mailutils.info.tar.gz\
 mailutils.dvi.gz\
 mailutils.ps.gz\
 mailutils.texi.tar.gz\
 mailutils.text

CLEANFILES = $(WEB_BIN)
clean-local:
	rm -rf $(WEB_HTML)

WEBDOC=$(WEB_HTML) $(WEB_BIN) 

webdocdir: $(WEBDOC) index.html
	if ! test -d $(WEBDOCDIR); then mkdir $(WEBDOCDIR); fi; \
	here=`cd $(srcdir) && pwd`; \
	webdocdir=`cd $(WEBDOCDIR) && pwd`; \
	for file in $(WEB_HTML) index.html; do \
	  if test -d $$here/$$file; then \
	    cp -pr $$here/$$file $$webdocdir/$$file; \
	  else \
	    test -f $$webdocdir/$$file \
	    || ln $$here/$$file $$webdocdir/$$file 2> /dev/null \
	    || cp -p $$here/$$file $$webdocdir/$$file || :; \
	  fi; \
	done; \
	if ! test -d $(WEBDOCDIR)/other; then mkdir $(WEBDOCDIR)/other; fi;\
	for file in $(WEB_BIN); do \
	  if test -d $$here/$$file; then \
	    cp -pr $$here/$$file $$webdocdir/other/$$file; \
	  else \
	    test -f $$webdocdir/other/$$file \
	    || ln $$here/$$file $$webdocdir/other/$$file 2> /dev/null \
	    || cp -p $$here/$$file $$webdocdir/other/$$file || :; \
	  fi; \
	done

index.html: index.html.in $(WEBDOC)
	@echo "s^%BASE_URL%^$(BASE_URL)^;" > .webdoc
	@echo "s/%DATE%/`date`/;" >> .webdoc
	@echo "s/%UPDATED%/`date +'%B, %d'`/;" >> .webdoc
	@echo "s/%PACKAGE_NAME%/$(PACKAGE_NAME)/" >> .webdoc
	@echo "s/%PACKAGE%/$(PACKAGE)/" >> .webdoc
	@echo "s/%VERSION%/$(VERSION)/" >> .webdoc
	@for file in `sed -n 's,.*"other/%PACKAGE%\(.*\)".*,$(PACKAGE)\1,pg;s,.*"\(.*\)%PACKAGE%\(.*\)".*,\1$(PACKAGE)\2,pg' index.html.in`; \
	do\
		ls -sk $$file; \
	done |\
         $(AWK) -vPACKAGE=$(PACKAGE) \
                'BEGIN { len = length(PACKAGE) } \
                 { gsub("\\.", "_", $$2); \
                   if (match($$2,"/")) \
                       $$2=substr($$2,RSTART+1); \
                   print "s/%" toupper(substr($$2,len+2)) "_SIZE%/" $$1 "/;" }' >> .webdoc
	sed -f .webdoc index.html.in > index.html
	rm -f .webdoc

webdocname=$(PACKAGE)-$(VERSION)-doc
WEBDOCDIR=$(webdocname)

webdoc: $(WEBDOC) index.html
	$(MAKE) WEBDOCDIR=$(top_builddir)/doc/texinfo/$(webdocname) webdocdir
	cd $(top_builddir)/doc/texinfo &&\
	tar cfz $(webdocname).tar.gz $(webdocname) &&\
	rm -r $(webdocname)
## End of webdoc
