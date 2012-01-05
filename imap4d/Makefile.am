## This file is part of GNU Mailutils.
## Copyright (C) 1999-2003, 2005, 2007-2012 Free Software Foundation,
## Inc.
##
## GNU Mailutils is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as
## published by the Free Software Foundation; either version 3, or (at
## your option) any later version.
##
## GNU Mailutils is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.

INCLUDES = @MU_APP_COMMON_INCLUDES@ @MU_AUTHINCS@ @SERV_AUTHINCS@ 
SUBDIRS = tests

sbin_PROGRAMS =	imap4d

if MU_COND_GSASL
 AUTH_GSASL = auth_gsasl.c
endif

if MU_COND_GSSAPI
 AUTH_GSSAPI = auth_gss.c
endif

imap4d_SOURCES = \
 append.c\
 authenticate.c\
 $(AUTH_GSASL)\
 $(AUTH_GSSAPI)\
 bye.c\
 capability.c\
 check.c\
 close.c\
 commands.c\
 copy.c\
 create.c\
 delete.c\
 examine.c\
 expunge.c\
 fetch.c\
 id.c\
 idle.c\
 imap4d.c\
 imap4d.h\
 io.c\
 list.c\
 logout.c\
 login.c\
 lsub.c\
 namespace.c\
 noop.c\
 parsebuf.c\
 preauth.c\
 quota.c\
 rename.c\
 search.c\
 select.c\
 signal.c\
 starttls.c\
 status.c\
 store.c\
 subscribe.c\
 sync.c\
 uid.c\
 unsubscribe.c\
 util.c

imap4d_LDADD = \
 ${MU_APP_LIBRARIES}\
 ${MU_LIB_MBOX}\
 ${MU_LIB_MH}\
 ${MU_LIB_MAILDIR}\
 ${MU_LIB_AUTH}\
 @MU_AUTHLIBS@ \
 ${MU_LIB_MAILUTILS}\
 @SERV_AUTHLIBS@ @MU_COMMON_LIBRARIES@ @TCPWRAP_LIBRARIES@

## This kludge is necessary to correctly establish imap4d -> MU_AUTHLIBS
## dependencies. Automake stupidly refuses to include them.
## The same kludge appears in pop3d/Makefile.am and auth/Makefile.am
## Think about better approach         --gray
imap4d_DEPENDENCIES = \
 @MU_AUTHLIBS_DEPENDENCY@\
 ../lib/libmuaux.a\
 ${MU_LIB_MBOX}\
 ${MU_LIB_MAILUTILS}
