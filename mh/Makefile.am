# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

bin_PROGRAMS = scan inc rmm refile mhpath folder rmf fmtcheck repl
noinst_LIBRARIES = libmh.a
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
INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib 
mh_LIBS = ./libmh.a ../mailbox/libmailbox.la ../lib/libmailutils.la
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
