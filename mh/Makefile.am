# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

bin_PROGRAMS = scan inc rmm
noinst_LIBRARIES = libmh.a
libmh_a_SOURCES= \
 mh_argp.c\
 mh_getopt.c\
 mh_error.c\
 mh_format.c\
 mh_init.c\
 mh_fmtgram.y
noinst_HEADERS = mh.h mh_getopt.h
scan_SOURCES = scan.c
inc_SOURCES = inc.c
rmm_SOURCES = rmm.c
INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib 
mh_LIBS = ./libmh.a ../mailbox/libmailbox.la ../lib/libmailutils.la
scan_LDADD = $(mh_LIBS)  -lcurses -ltermcap
inc_LDADD = $(mh_LIBS)
rmm_LDADD = $(mh_LIBS)
YFLAGS=-vt

