# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

noinst_PROGRAMS = \
 addr\
 url-parse\
 sfrom\
 mimetest\
 muemail\
 msg-send\
 http\
 mta

moddir=$(libdir)/$(PACKAGE)
mod_LTLIBRARIES = numaddr.la

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib
AM_LDFLAGS = ../mailbox/libmailbox.la

numaddr_la_SOURCES = numaddr.c
numaddr_la_LIBADD = ../libsieve/libsieve.la 
numaddr_la_LDFLAGS = -module -avoid-version -no-undefined

EXTRA_DIST = comsat.conf dot.biffrc gnu-imap4d.pam gnu-pop3d.pam\
 mailutils.rc reply.scm


