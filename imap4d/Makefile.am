## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 1999, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.
##
##   GNU Mailutils is free software; you can redistribute it and/or
##   modify it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 2, or (at
##   your option) any later version.
##
##   GNU Mailutils is distributed in the hope that it will be useful, but
##   WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##   General Public License for more details.
##
##   You should have received a copy of the GNU General Public License
##   along with GNU Mailutils; if not, write to the Free Software
##   Foundation, Inc.
##   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/include @AUTHINCS@ @INTLINCS@
SUBDIRS = testsuite

sbin_PROGRAMS =	imap4d

imap4d_LDADD = @AUTHOBJS@ ../mailbox/libmailbox.la  @AUTHLIBS@ ../lib/libmailutils.la @LTLIBINTL@
imap4d_SOURCES = append.c authenticate.c bye.c capability.c check.c close.c \
 commands.c copy.c create.c delete.c examine.c expunge.c fetch.c imap4d.c \
 imap4d.h list.c logout.c login.c lsub.c namespace.c noop.c rename.c search.c \
 select.c signal.c starttls.c status.c store.c subscribe.c sync.c uid.c \
 unsubscribe.c util.c version.c

## This kludge is necessary to correctly establish imap4d -> AUTHOBJS
## dependency. Think about better approach         --gray
imap4d_DEPENDENCIES = @AUTHOBJS@ ../mailbox/libmailbox.la ../lib/libmailutils.la

EXTRA_DIST=auth_gss.c
