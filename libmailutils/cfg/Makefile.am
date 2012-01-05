# GNU Mailutils -- a suite of utilities for electronic mail
# Copyright (C) 2010-2012 Free Software Foundation, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General
# Public License along with this library.  If not, see
# <http://www.gnu.org/licenses/>.

noinst_LTLIBRARIES = libcfg.la

libcfg_la_SOURCES = \
 driver.c\
 format.c\
 lexer.c\
 parser.c\
 gocs.c

localedir = $(datadir)/locale
AM_CPPFLAGS = \
 -DSYSCONFDIR=\"$(sysconfdir)\"\
 -DLOCALEDIR=\"$(localedir)\"

INCLUDES = @MU_LIB_COMMON_INCLUDES@ -I/libmailutils

EXTRA_DIST = \
 lexer.l\
 parser.y\
 parser.h

BUILT_SOURCES = parser.c parser.h lexer.c

YLWRAP = $(SHELL) $(mu_aux_dir)/gylwrap
AM_YFLAGS=-vt
AM_LFLAGS=-dvp

parser.c parser.h: $(srcdir)/parser.y
	$(YLWRAP) "$(YACC) $(AM_YFLAGS) -d" $< \
	          y.tab.c parser.c y.tab.h parser.h \
                  y.output parser.y.output \
                  -- -yy mu_cfg_yy

lexer.c: $(srcdir)/lexer.l parser.h
	$(YLWRAP) "$(LEX) $(AM_LFLAGS) $(LFLAGS)" \
                  $(srcdir)/lexer.l lex.yy.c lexer.c \
                  -- -yy mu_cfg_yy
