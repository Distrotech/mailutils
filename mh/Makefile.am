## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2001, 2002, 2003 Free Software Foundation, Inc.
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
##   Foundation, Inc.  51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

# All MH programs. Note that we have to explicitely append EXEEXT, Makefile
# won't do that.
PROGRAMS_MH = \
 ali$(EXEEXT)\
 anno$(EXEEXT)\
 burst$(EXEEXT)\
 comp$(EXEEXT)\
 fmtcheck$(EXEEXT)\
 folder$(EXEEXT)\
 forw$(EXEEXT)\
 inc$(EXEEXT)\
 install-mh$(EXEEXT)\
 mark$(EXEEXT)\
 mhl$(EXEEXT)\
 mhn$(EXEEXT)\
 mhparam$(EXEEXT)\
 mhpath$(EXEEXT)\
 pick$(EXEEXT)\
 refile$(EXEEXT)\
 repl$(EXEEXT)\
 rmf$(EXEEXT)\
 rmm$(EXEEXT)\
 scan$(EXEEXT)\
 send$(EXEEXT)\
 sortm$(EXEEXT)\
 whatnow$(EXEEXT)\
 whom$(EXEEXT)

EXTRA_PROGRAMS = \
 ali\
 anno\
 burst\
 comp\
 fmtcheck\
 folder\
 forw\
 inc\
 install-mh\
 mark\
 mhl\
 mhn\
 mhparam\
 mhpath\
 pick\
 refile\
 repl\
 rmf\
 rmm\
 scan\
 send\
 sortm\
 whatnow\
 whom

LIBRARIES_MH = libmh.a
EXTRA_LIBRARIES = libmh.a

bindir = @MH_BIN_DIR@
mhlibdir = $(pkgdatadir)/mh
bin_PROGRAMS = @BUILD_MH_PROGRAMS@
noinst_LIBRARIES = @BUILD_MH_LIBRARIES@

libmh_a_SOURCES= \
 compcommon.c\
 mh_alias_gram.c\
 mh_alias_lex.c\
 mh_argp.c\
 mh_ctx.c\
 mh_getopt.c\
 mh_global.c\
 mh_error.c\
 mh_format.c\
 mh_init.c\
 mh_list.c\
 mh_fmtgram.c\
 mh_msgset.c\
 mh_sequence.c\
 mh_stream.c\
 mh_whatnow.c\
 mh_whom.c
noinst_HEADERS = mh.h mh_getopt.h mh_alias.h

LISPSRC = mailutils-mh.el
lisp_LISP=@lisp_LISP@
EXTRA_LISP = mailutils-mh.el

BUILT_SOURCES= \
	mh_fmtgram.c \
        pick-gram.c \
        pick-gram.h \
        mh_alias_gram.c \
        mh_alias_lex.c \
        mh_alias.h

MAINTAINERCLEANFILES=$(BUILT_SOURCES)

mhlib_DATA = components replcomps replgroupcomps mhl.format $(LISPSRC)
EXTRA_DIST = components replcomps replgroupcomps mhl.format\
 mailutils-mh.eli mh_fmtgram.y pick.y mh_alias.y mh_alias.l
DISTCLEANFILES = mailutils-mh.el

SUFFIXES = .eli .el
mailutils-mh.el: mailutils-mh.eli

.eli.el:
	sed "s,BINDIR,$(bindir),g;s,MHLIBDIR,$(mhlibdir),g" $< > $@

INCLUDES = @MU_COMMON_INCLUDES@ @INTLINCS@
AM_CPPFLAGS = -D_GNU_SOURCE -DMHLIBDIR=\"$(mhlibdir)\" -DMHBINDIR=\"$(bindir)\"
mh_LIBS = \
 ./libmh.a\
 ../lib/libmuaux.la\
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/imap/libmu_imap.la\
 ../mailbox/pop/libmu_pop.la\
 ../mailbox/mh/libmu_mh.la\
 ../mailbox/nntp/libmu_nntp.la\
 ../mailbox/maildir/libmu_maildir.la\
 @MU_AUTHLIBS@\
 ../mailbox/libmailutils.la\
 @MU_COMMON_LIBRARIES@
LDADD = $(mh_LIBS)
scan_LDADD = $(mh_LIBS) @CURSES_LIBS@
pick_SOURCES = pick.c pick.h pick-gram.c pick-gram.h

YLWRAP = $(SHELL) $(top_srcdir)/scripts/ylwrap
AM_YFLAGS=-vt

pick-gram.c pick-gram.h: $(srcdir)/pick.y
	$(YLWRAP) "$(YACC) $(AM_YFLAGS) -d" $< \
	          y.tab.c pick-gram.c y.tab.h pick-gram.h \
                  y.output pick.output \
                  -- -yy pick_yy

mh_fmtgram.c: $(srcdir)/mh_fmtgram.y
	$(YLWRAP) "$(YACC) $(AM_YFLAGS)" $< \
	          y.tab.c mh_fmtgram.c y.output fmtgram.y.output \
                  -- -yy fmt_yy

mh_alias_gram.c mh_alias.h: $(srcdir)/mh_alias.y
	$(YLWRAP) "$(YACC) $(AM_YFLAGS) -d" $< \
	          y.tab.c mh_alias_gram.c y.tab.h mh_alias.h \
                  y.output mh_alias.output \
                  -- -yy ali_yy

mh_alias_lex.c: $(srcdir)/mh_alias.l mh_alias.h
	$(YLWRAP) "$(LEX) $(AM_LEXFLAGS) $(LEXFLAGS)" \
                  $(srcdir)/mh_alias.l lex.yy.c mh_alias_lex.c \
                  -- -yy ali_yy

install-exec-hook: @BUILD_MH_EXEC_HOOK@

mh_finish_install:
	@here=`pwd`; \
	cd $(DESTDIR)$(bindir); \
	rm -f folders; \
	$(LN_S) folder folders; \
	cd $$here
