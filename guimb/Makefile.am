# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

AUTOMAKE_OPTIONS = ../lib/ansi2knr
INCLUDES = -I. -I$(top_builddir) -I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/include \
 -I$(top_srcdir)/libmu_scm 

SUBDIRS=scm

bin_PROGRAMS=@GUIMB@
EXTRA_PROGRAMS=guimb
guimb_SOURCES=main.c collect.c util.c guimb.h 
guimb_LDADD = ../libmu_scm/libmu_scm.a ../mailbox/libmailbox.la \
 ../lib/libmailutils.la @GUILE_LIBS@ 

