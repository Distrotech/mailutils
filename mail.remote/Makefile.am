# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

AUTOMAKE_OPTIONS = ../lib/ansi2knr
INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/include -I$(top_srcdir)/libmu_scm 

SMTPPROGRAMS = mail.remote
EXTRA_PROGRAMS = mail.remote
libexec_PROGRAMS = @SMTPPROGRAMS@
mail_remote_SOURCES = mail.remote.c 

mail_remote_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.la

