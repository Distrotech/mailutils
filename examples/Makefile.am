## This file is part of GNU Mailutils.
## Copyright (C) 2002-2004, 2007-2012 Free Software Foundation, Inc.
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

if MU_COND_SUPPORT_NNTP
  NNTPCLIENT = nntpclient
endif

if MU_COND_SUPPORT_CXX
  CPP_DIR = cpp
endif

SUBDIRS = config $(CPP_DIR) python scheme

noinst_PROGRAMS = \
 aclck\
 addr\
 base64\
 echosrv\
 header\
 http\
 iconv\
 lsf\
 mblconv\
 msg-send\
 mta\
 mucat\
 muauth\
 muemail\
 murun\
 musocio\
 $(NNTPCLIENT)\
 sa\
 sfrom

EXTRA_PROGRAMS = nntpclient

## NOTE: Numaddr must be an installable target, otherwise libtool
## will not create a shared library and `make check' will fail in
## sieve. Pity.
## -Sergey
moddir=@MU_SIEVE_MODDIR@
mod_LTLIBRARIES = numaddr.la

INCLUDES = @MU_LIB_COMMON_INCLUDES@ 

LDADD = \
 ../lib/libmuaux.a\
 ${MU_LIB_MAILUTILS}\
 @MU_COMMON_LIBRARIES@

numaddr_la_SOURCES = numaddr.c
numaddr_la_LIBADD = ${MU_LIB_SIEVE}
numaddr_la_LDFLAGS = -module -avoid-version -no-undefined -rpath '$(moddir)'

msg_send_LDADD =\
 ../lib/libmuaux.a\
 ${MU_LIB_MAILER}\
 @MU_AUTHLIBS@\
 ${MU_LIB_MAILUTILS}

mta_CPPFLAGS=-I${top_srcdir}/lib -I${top_builddir}/lib
mta_LDADD =\
 ../lib/libmuaux.a\
 ${MU_LIB_MBOX}\
 ${MU_LIB_MAILER}\
 ${MU_LIB_MAILUTILS}

sfrom_LDADD =\
 ../lib/libmuaux.a\
 ${MU_LIB_MBOX}\
 ${MU_LIB_IMAP}\
 ${MU_LIB_POP}\
 ${MU_LIB_NNTP}\
 ${MU_LIB_MH}\
 ${MU_LIB_MAILDIR}\
 ${MU_LIB_AUTH}\
 @MU_AUTHLIBS@\
 ${MU_LIB_MAILUTILS}

nntpclient_LDADD = \
 ../lib/libmuaux.a\
 ${MU_LIB_NNTP}\
 ${MU_LIB_AUTH}\
 @MU_AUTHLIBS@\
 ${MU_LIB_MAILUTILS}\
 @READLINE_LIBS@

lsf_LDADD = \
 ${MU_LIB_MBOX}\
 ${MU_LIB_IMAP}\
 ${MU_LIB_POP}\
 ${MU_LIB_NNTP}\
 ${MU_LIB_MH}\
 ${MU_LIB_MAILDIR}\
 ${MU_LIB_AUTH}\
 @MU_AUTHLIBS@ \
 ${MU_LIB_MAILUTILS}

muauth_CPPFLAGS = @MU_APP_COMMON_INCLUDES@
muauth_LDADD = \
 ${MU_APP_LIBRARIES}\
 ${MU_LIB_AUTH}\
 @MU_AUTHLIBS@ \
 ${MU_LIB_MAILUTILS}

muemail_CPPFLAGS = @MU_APP_COMMON_INCLUDES@
muemail_LDADD = \
 ${MU_APP_LIBRARIES} \
 ${MU_LIB_MAILUTILS}

