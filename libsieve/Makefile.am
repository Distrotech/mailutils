## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2002 Free Software Foundation, Inc.
##
##   GNU Mailtuils is free software; you can redistribute it and/or
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

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib @INTLINCS@
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


