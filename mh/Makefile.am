## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2001, 2002 Free Software Foundation, Inc.
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

PROGRAMS_MH = scan inc rmm refile mhpath folder rmf fmtcheck repl whatnow
EXTRA_PROGRAMS = scan inc rmm refile mhpath folder rmf fmtcheck repl whatnow

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
 mh_fmtgram.y\
 mh_msgset.c\
 mh_whatnow.c
noinst_HEADERS = mh.h mh_getopt.h

mhlib_DATA = replcomps
EXTRA_DIST = replcomps

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib -I${top_srcdir}/mailbox @INTLINCS@
AM_CPPFLAGS = -DMHLIBDIR=\"$(mhlibdir)\"
mh_LIBS = ./libmh.a ../mailbox/libmailbox.la ../lib/libmailutils.la @LTLIBINTL@
scan_LDADD = $(mh_LIBS) @CURSES_LIBS@
inc_LDADD = $(mh_LIBS)
rmm_LDADD = $(mh_LIBS)
refile_LDADD = $(mh_LIBS)
mhpath_LDADD = $(mh_LIBS)
folder_LDADD = $(mh_LIBS)
rmf_LDADD = $(mh_LIBS)
fmtcheck_LDADD = $(mh_LIBS)
repl_LDADD = $(mh_LIBS)
whatnow_LDADD = $(mh_LIBS)

AM_YFLAGS=-vt

install-exec-hook: @BUILD_MH_EXEC_HOOK@
	
mh_finish_install:
	@here=`pwd`; \
	cd $(DESTDIR); \
	-rm -f $(bindir)/folders; \
	$(LN_S) $(bindir)/folder $(bindir)/folders; \
	cd $$here
