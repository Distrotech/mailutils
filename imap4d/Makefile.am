# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/include @AUTHINCS@
SUBDIRS = testsuite

sbin_PROGRAMS =	imap4d

imap4d_LDADD = @AUTHOBJS@ ../mailbox/libmailbox.la  @AUTHLIBS@ ../lib/libmailutils.la 
imap4d_SOURCES = append.c authenticate.c bye.c capability.c check.c close.c \
 commands.c copy.c create.c delete.c examine.c expunge.c fetch.c imap4d.c \
 imap4d.h list.c logout.c login.c lsub.c namespace.c noop.c rename.c search.c \
 select.c signal.c status.c store.c subscribe.c sync.c uid.c unsubscribe.c \
 util.c version.c

## This kludge is necessary to correctly establish imap4d -> AUTHOBJS
## dependency. Think about better approach         --gray
imap4d_DEPENDENCIES = @AUTHOBJS@ ../mailbox/libmailbox.la ../lib/libmailutils.la

EXTRA_DIST=auth_gss.c
