## This file is part of GNU Mailutils.
## Copyright (C) 1999-2002, 2005, 2007-2012, 2014-2015 Free Software
## Foundation, Inc.
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

SUBDIRS = gnu .

AM_CPPFLAGS = \
 @MU_LIB_COMMON_INCLUDES@\
 -I${top_srcdir}/lib/gnu\
 -I${top_builddir}/lib/gnu\
 @GUILE_INCLUDES@\
 @PYTHON_INCLUDES@

lib_LTLIBRARIES = libmuaux.la
noinst_LIBRARIES = libmuscript.a libmutcpwrap.a

libmuaux_la_SOURCES = \
 argp_base.c\
 daemon.c\
 mailcap.c\
 manlock.c\
 signal.c\
 strexit.c\
 mu_umaxtostr.c\
 mu_umaxtostr.h
libmuaux_la_LIBADD=gnu/libgnu.la
libmuaux_la_LDFLAGS = -version-info @VI_CURRENT@:@VI_REVISION@:@VI_AGE@

libmutcpwrap_a_SOURCES = tcpwrap.c

noinst_HEADERS =\
 mailcap.h\
 muaux.h\
 muscript.h\
 muscript_priv.h\
 tcpwrap.h

EXTRA_DIST = utmp.c

libmuscript_a_SOURCES = \
 script.c\
 sieve.c

if MU_COND_LIBMU_SCM
 libmuscript_a_SOURCES += guile.c
endif

if MU_COND_PYTHON
 libmuscript_a_SOURCES += python.c
endif

