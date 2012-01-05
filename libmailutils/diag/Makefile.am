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

noinst_LTLIBRARIES = libdiag.la

libdiag_la_SOURCES = \
 bt.c\
 debug.c\
 diag.c\
 muerror.c\
 muerrno.c\
 syslog.c\
 wd.c

INCLUDES = @MU_LIB_COMMON_INCLUDES@ -I/libmailutils

EXTRA_DIST = \
 debcat\
 errors\
 muerrno.cin

BUILT_SOURCES = muerrno.c

muerrno.c: errors muerrno.cin
	$(AWK) -f $(mu_aux_dir)/generr.awk $^ > $@



