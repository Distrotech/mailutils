# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/mailbox/include -I${top_srcdir}/lib
AM_CFLAGS = -DSITE_VIRTUAL_PWDDIR=\"@SITE_VIRTUAL_PWDDIR@\"

lib_LTLIBRARIES = libmuauth.la

libmuauth_la_SOURCES = \
pam.c \
sql.c \
virtual.c 

libmuauth_la_LDFLAGS = -version-info 0:0:0
