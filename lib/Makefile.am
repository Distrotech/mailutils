## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
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
##   Foundation, Inc.
##   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

noinst_LTLIBRARIES = libmailutils.la

INCLUDES = -I${top_srcdir}/include -I${top_builddir}/include/mailutils/gnu @INTLINCS@
libmailutils_la_SOURCES = basename.c daemon.c getopt.c getopt1.c md5.c \
                         mu_dbm.c xstrdup.c xmalloc.c 

EXTRA_DIST = alloca.c fnmatch.c getpass.c malloc.c obstack.c \
 realloc.c setenv.c snprintf.c strncasecmp.c \
 strcasecmp.c strsignal.c xstrtol.c  vasprintf.c vsyslog.c \
 utmp.c 

noinst_HEADERS = error.h fnmatch.h md5.h \
 mu_dbm.h\
 snprintf.h xalloc.h xstrtol.h obstack.h \
 mu_asprintf.h

libmailutils_la_LIBADD = @LTLIBOBJS@ @ALLOCA@

