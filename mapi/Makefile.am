# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

AUTOMAKE_OPTIONS = ../lib/ansi2knr

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/mailbox/include

SUBDIRS = include

lib_LTLIBRARIES = libmapi.la

/* FIXME:  This should be part of the default includes.  */
EXTRA_DIST = mapi.h

libmapi_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.la

libmapi_la_SOURCES = \
 MAPIAddress.c \
 MAPIDeleteMail.c \
 MAPIDetails.c \
 MAPIFindNext.c \
 MAPIFreeBuffer.c \
 MAPILogoff.c \
 MAPILogon.c \
 MAPIReadMail.c \
 MAPISaveMail.c \
 MAPISendDocuments.c \
 MAPISendMail.c

libmapi_la_LDFLAGS = -version-info 0:0:0
