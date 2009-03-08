## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2009 Free Software Foundation, Inc.
##
##   GNU Mailutils is free software; you can redistribute it and/or
##   modify it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 3, or (at
##   your option) any later version.
##
##   This program is distributed in the hope that it will be useful, but
##   WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##   General Public License for more details.
##
##   You should have received a copy of the GNU General Public License
##   along with this program; if not, write to the Free Software
##   Foundation, Inc.  51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

INCLUDES = @MU_COMMON_INCLUDES@ $(PYTHON_INCLUDES)

MU_PYTHON_LTLIBS = c_api.la
EXTRA_LTLIBRARIES = c_api.la
pkgpyexec_LTLIBRARIES = @MU_PYTHON_LTLIBS@
c_api_la_LDFLAGS = -avoid-version -module -rpath $(pkgpyexecdir)
c_api_la_LIBADD = $(PYTHON_LIBS) @MU_COMMON_LIBRARIES@ \
 ${MU_LIB_MBOX}\
 ${MU_LIB_IMAP}\
 ${MU_LIB_POP}\
 ${MU_LIB_NNTP}\
 ${MU_LIB_MH}\
 ${MU_LIB_MAILDIR}\
 ${MU_LIB_AUTH}\
 ${MU_LIB_MAILER}\
 @MU_AUTHLIBS@\
 ${MU_LIB_MAILUTILS}

c_api_la_SOURCES = \
 c_api.c \
 c_api.h \
 error.c \
 address.c \
 address-private.h \
 attribute.c \
 attribute-private.h \
 auth.c \
 auth-private.h \
 body.c \
 body-private.h \
 debug.c \
 debug-private.h \
 envelope.c \
 envelope-private.h \
 filter.c \
 folder.c \
 folder-private.h \
 list.c \
 list-private.h \
 header.c \
 header-private.h \
 mailer.c \
 mailer-private.h \
 mailbox.c \
 mailbox-private.h \
 mailcap.c \
 mailcap-private.h \
 message.c \
 message-private.h \
 mime.c \
 mime-private.h \
 stream.c \
 stream-private.h \
 registrar.c \
 url.c \
 url-private.h \
 util.c

