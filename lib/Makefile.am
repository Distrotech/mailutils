## This file is part of GNU Mailutils.
## Copyright (C) 1999-2002, 2005, 2007-2012 Free Software Foundation,
## Inc.
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

include gnulib.mk

INCLUDES = @MU_LIB_COMMON_INCLUDES@ 

libmuaux_a_SOURCES += \
 argp_base.c\
 daemon.c\
 mailcap.c\
 manlock.c\
 signal.c\
 strexit.c\
 tcpwrap.c\
 mu_umaxtostr.c\
 mu_umaxtostr.h

noinst_HEADERS +=\
 mailcap.h\
 muaux.h\
 tcpwrap.h

EXTRA_DIST += utmp.c
gl_LIBOBJS += @LIBOBJS@
gl_LTLIBOBJS += @LTLIBOBJS@


