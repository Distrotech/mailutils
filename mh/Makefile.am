## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2001, 2002, 2003 Free Software Foundation, Inc.
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

PROGRAMS_MH = \
 anno\
 comp\
 fmtcheck\
 folder\
 inc\
 install-mh\
 mark\
 mhl\
 mhpath\
 refile\
 repl\
 rmf\
 rmm\
 scan\
 send\
 whatnow

EXTRA_PROGRAMS = \
 anno\
 comp\
 fmtcheck\
 folder\
 inc\
 install-mh\
 mark\
 mhl\
 mhpath\
 refile\
 repl\
 rmf\
 rmm\
 scan\
 send\
 whatnow

LIBRARIES_MH = libmh.a
EXTRA_LIBRARIES = libmh.a

mhlibdir = $(pkgdatadir)/mh
bin_PROGRAMS = @BUILD_MH_PROGRAMS@
noinst_LIBRARIES = @BUILD_MH_LIBRARIES@

libmh_a_SOURCES= \
 mh_argp.c\
 mh_ctx.c\
 mh_getopt.c\
 mh_global.c\
 mh_error.c\
 mh_format.c\
 mh_init.c\
 mh_list.c\
 mh_fmtgram.y\
 mh_msgset.c\
 mh_whatnow.c
noinst_HEADERS = mh.h mh_getopt.h

mhlib_DATA = replcomps mhl.format 
EXTRA_DIST = replcomps mhl.format 

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib -I${top_srcdir}/mailbox @INTLINCS@
AM_CPPFLAGS = -DMHLIBDIR=\"$(mhlibdir)\"
mh_LIBS = ./libmh.a ../mailbox/libmailbox.la ../lib/libmailutils.la @LTLIBINTL@
LDADD = $(mh_LIBS)
scan_LDADD = $(mh_LIBS) @CURSES_LIBS@

AM_YFLAGS=-vt

install-exec-hook: @BUILD_MH_EXEC_HOOK@
	
mh_finish_install:
	@here=`pwd`; \
	cd $(DESTDIR); \
	rm -f $(bindir)/folders; \
	$(LN_S) $(bindir)/folder $(bindir)/folders; \
	cd $$here
