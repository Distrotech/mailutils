# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib 
# SUBDIRS  = testsuite
bin_PROGRAMS = sieve

sieve_SOURCES = sieve.c
sieve_LDADD = ../libsieve/libsieve.la ../mailbox/libmailbox.la ../lib/libmailutils.la
