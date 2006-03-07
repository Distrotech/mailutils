## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2005 Free Software Foundation, Inc.
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
##   Foundation, Inc. 51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

INCLUDES = @MU_COMMON_INCLUDES@ @INTLINCS@

bin_PROGRAMS = mailutils-config
AM_CPPFLAGS = -DGNULIB_ARGP_EXTERN_BASENAME=1

mailutils_config_SOURCES = mailutils-config.c
mailutils_config_LDADD = \
 ../lib/libmuaux.la \
 ${MU_LIB_MAILUTILS} \
 @MU_COMMON_LIBRARIES@

mailutils_config_CFLAGS = -DCOMPILE_FLAGS="\"-I$(includedir)\"" \
  -I$(top_srcdir)/intl \
  -I${top_srcdir}/lib \
  -DLINK_FLAGS="\"-L$(libdir)\"" \
  -DLINK_POSTFLAGS="\"$(LTLIBICONV)\"" \
  -DAUTHLIBS="\"$(MU_AUTHLIBS)\"" \
  -DGUILE_LIBS="\"$(GUILE_LIBS)\"" \
  -DI18NLIBS="\"`test $(BUILD_INCLUDED_LIBINTL) = no && echo $(LIBINTL)`\""
