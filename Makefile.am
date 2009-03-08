## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2000,2001,2002,2003,2004,2005,2006,2007,2008,2009
##   Free Software Foundation, Inc.
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
##   Foundation, Inc.  51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

AUTOMAKE_OPTIONS = gnits 1.8.5 dist-bzip2 std-options
ACLOCAL_AMFLAGS = -I m4 -I am

SUBDIRS = \
 include\
 po\
 testsuite\
 mailbox\
 sql\
 libmu_auth\
 libproto\
 lib\
 libmu_argp\
 libmu_cfg\
 libmu_cpp\
 libmu_scm\
 libmu_sieve\
 doc\
 config\
 examples\
 frm\
 pop3d\
 imap4d\
 maidag\
 mail\
 sieve\
 scripts\
 guimb\
 messages\
 comsat\
 readmsg\
 mail.local\
 mail.remote\
 dotlock\
 mh\
 movemail\
 mimeview\
 python

EXTRA_DIST = COPYING.LESSER

.PHONY: make-ChangeLog
make-ChangeLog: 
	if test -d .git; then                                           \
	  $(top_srcdir)/scripts/gitlog-to-changelog |                   \
            sed '/<unknown>$$/d' | fmt -s > $(distdir)/cl-t;            \
	  echo "" >> $(distdir)/cl-t;                                 \
	  echo "Local Variables:" >> $(distdir)/cl-t;                   \
	  echo "mode: change-log" >> $(distdir)/cl-t;                   \
	  echo "version-control: never"  >> $(distdir)/cl-t;            \
	  echo "buffer-read-only: t" >> $(distdir)/cl-t;                \
	  echo "End:" >> $(distdir)/cl-t;                               \
	  rm -f $(distdir)/ChangeLog;                                   \
	  mv $(distdir)/cl-t $(distdir)/ChangeLog;                      \
	fi

dist-hook: make-ChangeLog

.PHONY: ChangeLog
ChangeLog: 
	if test -d .git; then                                           \
	  $(top_srcdir)/scripts/gitlog-to-changelog |                   \
            sed '/<unknown>$$/d' | fmt -s > ChangeLog;                  \
	  echo "" >> ChangeLog;                                       \
	  echo "Local Variables:" >> ChangeLog;                         \
	  echo "mode: change-log" >> ChangeLog;                         \
	  echo "version-control: never"  >> ChangeLog;                  \
	  echo "buffer-read-only: t" >> ChangeLog;                      \
	  echo "End:" >> ChangeLog;                                     \
        fi

alpha:
	$(MAKE) dist distdir=$(PACKAGE)-$(VERSION)-`date +"%Y%m%d"`

alphacheck:
	$(MAKE) distcheck distdir=$(PACKAGE)-$(VERSION)-`date +"%Y%m%d"`

rpm: dist
	rpm -ta --clean mailutils-$(VERSION).tar.gz

