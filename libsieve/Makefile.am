# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib 
YLWRAP = $(SHELL) $(top_srcdir)/scripts/ylwrap
AM_YFLAGS = -dtv
AM_CPPFLAGS = -DSIEVE_MODDIR=\"@SIEVE_MODDIR@\"

lib_LTLIBRARIES = libsieve.la

libsieve_la_SOURCES = \
 actions.c\
 argp.c\
 comparator.c\
 load.c\
 prog.c\
 register.c\
 require.c\
 runtime.c\
 sieve-gram.c\
 sieve-gram.h\
 sieve-lex.c\
 tests.c\
 util.c
libsieve_la_LIBADD=@LTDL_LIB@

noinst_HEADERS = sieve.h

BUILT_SOURCES= \
  sieve-gram.c sieve-gram.h \
  sieve-lex.c

MAINTAINERCLEANFILES=$(BUILT_SOURCES)

EXTRA_DIST = sieve.y sieve.l

sieve-lex.c: $(srcdir)/sieve.l sieve-gram.h
	$(YLWRAP) "$(LEX) $(AM_LEXFLAGS) $(LEXFLAGS)" \
                  $(srcdir)/sieve.l lex.yy.c sieve-lex.c \
                  -- -yy sieve_yy

sieve-gram.c sieve-gram.h: $(srcdir)/sieve.y
	$(YLWRAP) "$(YACC) $(AM_YFLAGS) $(YFLAGS)" $(srcdir)/sieve.y \
	          y.tab.c sieve-gram.c y.tab.h sieve-gram.h y.output y.output \
                  -- -yy sieve_yy


