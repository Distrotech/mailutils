# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

AUTOMAKE_OPTIONS = ../lib/ansi2knr
noinst_PROGRAMS = addr url-parse sfrom mimetest muemail msg-send

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib
AM_LDFLAGS = ../mailbox/libmailbox.la ../lib/libmailutils.la 

EXTRA_DIST = comsat.conf dot.biffrc gnu-imap4d.pam gnu-pop3d.pam\
 mailutils.rc reply.scm
