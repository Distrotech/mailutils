## This file is part of GNU Mailutils
## Copyright (C) 2000-2012 Free Software Foundation, Inc.
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

ACLOCAL_AMFLAGS = -I m4 -I am -I gint -I doc/imprimatur

if MU_COND_PYTHON
  PYTHON_DIR = python
endif

if MU_COND_MH
  MH_DIR = mh
endif

if MU_COND_FRM
  FRM_DIR = frm
endif

if MU_COND_POP3D
  POP3D_DIR = pop3d
endif

if MU_COND_IMAP4D
  IMAP4D_DIR = imap4d
endif

if MU_COND_MAIDAG
  MAIDAG_DIR = maidag
endif

if MU_COND_MAIL
  MAIL_DIR = mail
endif

if MU_COND_SIEVE
  SIEVE_DIR = sieve
endif

if MU_COND_MESSAGES
  MESSAGES_DIR = messages
endif

if MU_COND_COMSAT
  COMSAT_DIR = comsat
endif

if MU_COND_READMSG
  READMSG_DIR = readmsg
endif

if MU_COND_DOTLOCK
  DOTLOCK_DIR = dotlock
endif

if MU_COND_MOVEMAIL
  MOVEMAIL_DIR = movemail
endif

if MU_COND_MIMEVIEW
  MIMEVIEW_DIR = mimeview
endif

if MU_COND_LIBMU_SCM
  GINT_DIR = gint
  LIBMU_SCM_DIR = libmu_scm
  SCHEME_DIR = scheme
endif

if MU_COND_SUPPORT_CXX
  LIBMU_CPP_DIR = libmu_cpp
endif

if MU_COND_DBM
  LIBMU_DBM_DIR = libmu_dbm
endif

SUBDIRS = . \
 mu-aux\
 include\
 po\
 libmailutils\
 sql\
 libmu_auth\
 libproto\
 libmu_compat\
 testsuite\
 lib\
 libmu_argp\
 libmu_cfg\
 $(LIBMU_CPP_DIR)\
 $(GINT_DIR)\
 $(LIBMU_SCM_DIR)\
 $(LIBMU_DBM_DIR)\
 libmu_sieve\
 $(PYTHON_DIR)\
 doc\
 examples\
 mu\
 $(SCHEME_DIR)\
 $(FRM_DIR)\
 $(POP3D_DIR)\
 $(IMAP4D_DIR)\
 $(MAIDAG_DIR)\
 $(MAIL_DIR)\
 $(SIEVE_DIR)\
 $(MESSAGES_DIR)\
 $(COMSAT_DIR)\
 $(READMSG_DIR)\
 $(DOTLOCK_DIR)\
 $(MH_DIR)\
 $(MOVEMAIL_DIR)\
 $(MIMEVIEW_DIR)

EXTRA_DIST = COPYING.LESSER paths git-describe git-describe.h
BUILT_SOURCES = git-describe git-describe.h
DISTCLEANFILES = pathdefs.h git-describe.h

gen_start_date = "2008-12-08"
prev_change_log = "doc/ChangeLog.CVS"
amend_file=ChangeLog.amend

.PHONY: ChangeLog
ChangeLog:
	@if test -d .git; then                                             \
	  cmd="$(top_srcdir)/build-aux/gitlog-to-changelog";               \
	  if test -n "$(amend_file)"; then                                 \
            cmd="$$cmd --amend=$(amend_file)";                             \
          fi;                                                              \
	  if test -n "$(gen_start_date)"; then                             \
	    cmd="$$cmd --since=\"$(gen_start_date)\"";                     \
	  fi;                                                              \
	  $$cmd --format='%s%n%n%b%n' |                                    \
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

.PHONY: git-describe
git-describe:
	$(AM_V_GEN)if test -d .git; then \
		dirty=`git diff-index --name-only HEAD 2>/dev/null` || dirty=;\
		test -n "$$dirty" && dirty="-dirty"; \
		descr=`git describe`; \
		echo $${descr}$$dirty > git-describe.tmp; \
		test -f git-describe && \
                  cmp git-describe.tmp git-describe >/dev/null || \
		  cp git-describe.tmp git-describe; \
	        rm git-describe.tmp; \
	fi

dist-hook: ChangeLog git-describe
	@PATCHLEV=`echo "$(PACKAGE_VERSION)" | \
                   sed -r "s/[0-9]+\.[0-9]+\.?//"`; \
	if test $${PATCHLEV:-0} -lt 50; then \
		if grep -q FIXME NEWS; then \
			echo >&2 "NEWS file contains FIXMEs"; \
			exit 1; \
		fi; \
	fi

git-describe.h: git-describe
	$(AM_V_GEN)if test -f $(srcdir)/git-describe; then \
	  sed '1s/.*/#define GIT_DESCRIBE "&"/' $(srcdir)/git-describe; \
	else \
          echo "/* No git tag */"; \
	fi > git-describe.h.tmp; \
	test -f git-describe.h && \
          cmp git-describe.h.tmp git-describe.h >/dev/null || \
          cp git-describe.h.tmp git-describe.h; \
	rm git-describe.h.tmp

alpha: git-describe
	$(AM_V_GEN)if test -f $(srcdir)/git-describe; then \
	  tag=`head -n 1 $(srcdir)/git-describe`; \
	else \
	  tag=`date +"%Y%m%d"`; \
	fi; \
	$(MAKE) dist distdir=$(PACKAGE)-$(VERSION)-$$tag

alphacheck:
	$(AM_V_GEN)if test -f $(srcdir)/git-describe; then \
	  tag=`head -n 1 $(srcdir)/git-describe`; \
	else \
	  tag=`date +"%Y%m%d"`; \
	fi; \
	$(MAKE) distcheck distdir=$(PACKAGE)-$(VERSION)-$$tag

