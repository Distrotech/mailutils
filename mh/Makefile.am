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

PROGRAMS_MH = scan inc rmm refile mhpath folder rmf fmtcheck repl
EXTRA_PROGRAMS = scan inc rmm refile mhpath folder rmf fmtcheck repl

LIBRARIES_MH = libmh.a
EXTRA_LIBRARIES = libmh.a

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
 mh_msgset.c
noinst_HEADERS = mh.h mh_getopt.h
scan_SOURCES = scan.c
inc_SOURCES = inc.c
rmm_SOURCES = rmm.c
rmf_SOURCES = rmf.c
refile_SOURCES = refile.c
mhpath_SOURCES = mhpath.c
folder_SOURCES = folder.c
fmtcheck_SOURCES = fmtcheck.c
repl_SOURCES = repl.c
INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib @INTLINCS@
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

AM_YFLAGS=-vt

install-exec-hook:
	-rm -f $(DESTDIR)$(bindir)/folders
	$(LN_S) $(DESTDIR)$(bindir)/folder $(DESTDIR)$(bindir)/folders
