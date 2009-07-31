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

ACLOCAL_AMFLAGS = -I m4 -I am

SUBDIRS = \
 mu-aux\
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
 python\
 doc\
 config\
 examples\
 frm\
 pop3d\
 imap4d\
 maidag\
 mail\
 sieve\
 guimb\
 messages\
 comsat\
 readmsg\
 dotlock\
 mh\
 movemail\
 mimeview

EXTRA_DIST = COPYING.LESSER paths

DISTCLEANFILES = pathdefs.h

gen_start_date = 
prev_change_log = 

.PHONY: ChangeLog
ChangeLog:
	@if test -d .git; then                                             \
	  cmd=$(top_srcdir)/build-aux/gitlog-to-changelog;                 \
	  if test -n "$(gen_start_date)"; then                             \
	    cmd="$$cmd --since=\"$(gen_start_date)\"";                     \
	  fi;                                                              \
	  $$cmd |                                                          \
            sed '/<unknown>$$/d' | fmt -s > cl-t;                          \
          if test -n "$(prev_change_log)" && test -f "$(prev_change_log)"; \
	  then                                                             \
	    echo "" >> cl-t;                                               \
	    cat "$(prev_change_log)" |                                     \
	      sed '/^Local Variables:/,/^End:/d' >> cl-t;                  \
	  fi;                                                              \
	  echo "Local Variables:" >> cl-t;                                 \
	  echo "mode: change-log" >> cl-t;                                 \
	  echo "version-control: never"  >> cl-t;                          \
	  echo "buffer-read-only: t" >> cl-t;                              \
	  echo "End:" >> cl-t;                                             \
	  rm -f ChangeLog;                                                 \
	  mv cl-t ChangeLog;                                               \
	fi

alpha:
	$(MAKE) dist distdir=$(PACKAGE)-$(VERSION)-`date +"%Y%m%d"`

alphacheck:
	$(MAKE) distcheck distdir=$(PACKAGE)-$(VERSION)-`date +"%Y%m%d"`

rpm: dist
	rpm -ta --clean mailutils-$(VERSION).tar.gz

