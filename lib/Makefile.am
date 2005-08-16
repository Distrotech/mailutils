## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 1999, 2000, 2001, 2002, 2005 Free Software Foundation, Inc.
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

noinst_LTLIBRARIES = libmuaux.la

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/mailbox -I${top_builddir}/include/mailutils/gnu @INTLINCS@
libmuaux_la_SOURCES = \
 daemon.c\
 mailcap.c\
 mu_dbm.c\
 xalloc_die.c

EXTRA_DIST = utmp.c 

noinst_HEADERS =\
 mu_dbm.h\
 mu_asprintf.h

libmuaux_la_LIBADD = @LTLIBOBJS@

BUILT_SOURCES=
MOSTLYCLEANFILES=

##:## EOF marker for gnulib-sync script. Please, do not remove ##:##
## Do not change anything below this line ##
libmuaux_la_SOURCES += allocsa.h allocsa.c
EXTRA_DIST += allocsa.valgrind


libmuaux_la_SOURCES += exit.h

libmuaux_la_SOURCES += exitfail.h exitfail.c

BUILT_SOURCES += $(STDBOOL_H)
EXTRA_DIST += stdbool_.h

# We need the following in order to create an <stdbool.h> when the system
# doesn't have one that works.
all-local $(libmuaux_la_OBJECTS): $(STDBOOL_H)
stdbool.h: stdbool_.h
	sed -e 's/@''HAVE__BOOL''@/$(HAVE__BOOL)/g' < $(srcdir)/stdbool_.h > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += stdbool.h stdbool.h-t

libmuaux_la_SOURCES += xalloc.h xmalloc.c

BUILT_SOURCES += $(FNMATCH_H)
EXTRA_DIST += fnmatch_.h fnmatch_loop.c

# We need the following in order to create an <fnmatch.h> when the system
# doesn't have one that supports the required API.
all-local $(libmuaux_la_OBJECTS): $(FNMATCH_H)
fnmatch.h: fnmatch_.h
	cp $(srcdir)/fnmatch_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += fnmatch.h fnmatch.h-t

libmuaux_la_SOURCES += getpass.h


libmuaux_la_SOURCES += mbswidth.h mbswidth.c



libmuaux_la_SOURCES += setenv.h

libmuaux_la_SOURCES += snprintf.h

libmuaux_la_SOURCES += xstrtol.h xstrtol.c xstrtoul.c

libmuaux_la_SOURCES += vasprintf.h

libmuaux_la_SOURCES += xsize.h

