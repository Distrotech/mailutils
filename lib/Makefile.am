# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

AUTOMAKE_OPTIONS = ansi2knr
noinst_LTLIBRARIES = libmailutils.la

SUBDIRS = posix

INCLUDES = -I${top_srcdir}/include
libmailutils_la_SOURCES = basename.c daemon.c getopt.c getopt1.c md5.c \
                         mu_dbm.c xstrdup.c xmalloc.c \
                         pin.c 

EXTRA_DIST = alloca.c fnmatch.c fgetpwent.c getpass.c malloc.c obstack.c \
  realloc.c setenv.c snprintf.c strncasecmp.c \
  strcasecmp.c strsignal.c xstrtol.c  vasprintf.c \
  utmp.c 

noinst_HEADERS = argcv.h error.h fnmatch.h getline.h getopt.h md5.h \
 mu_dbm.h\
 regex.h snprintf.h xalloc.h xstrtol.h obstack.h \
 argp-fmtstream.h argp-namefrob.h argp.h mu_asprintf.h

libmailutils_la_LIBADD = @LTLIBOBJS@ @ALLOCA@

