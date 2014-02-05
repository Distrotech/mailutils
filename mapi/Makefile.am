## This file is part of GNU Mailutils.
## Copyright (C) 2001-2002, 2007, 2010-2012, 2014 Free Software
## Foundation, Inc.
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

INCLUDES = @MU_LIB_COMMON_INCLUDES@

SUBDIRS = include

lib_LTLIBRARIES = libmapi.la

/* FIXME:  This should be part of the default includes.  */
EXTRA_DIST = mapi.h

libmapi_LDADD = ../lib/libmuaux.a ${MU_LIB_MAILUTILS} 

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

libmapi_la_LDFLAGS = -version-info @VI_CURRENT@:@VI_REVISION@:@VI_AGE@
