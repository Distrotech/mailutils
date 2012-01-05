## This file is part of GNU Mailutils.
## Copyright (C) 2003, 2005-2007, 2010-2012 Free Software Foundation,
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

INCLUDES = @MU_LIB_COMMON_INCLUDES@ 

lib_LTLIBRARIES = libmu_imap.la
libmu_imap_la_LDFLAGS=-version-info @VI_CURRENT@:@VI_REVISION@:@VI_AGE@
libmu_imap_la_LIBADD = ${MU_LIB_AUTH} ${MU_LIB_MAILUTILS} @INTLLIBS@ 

SUBDIRS = . tests

libmu_imap_la_SOURCES = \
 appmsg.c\
 appstr.c\
 appstrsiz.c\
 fetch.c\
 gencom.c\
 genlist.c\
 callback.c\
 capability.c\
 capatst.c\
 carrier.c\
 check.c\
 close.c\
 connect.c\
 copy.c\
 create.c\
 delete.c\
 destroy.c\
 disconnect.c\
 err.c\
 expunge.c\
 id.c\
 list.c\
 login.c\
 logout.c\
 lsub.c\
 mbcreate.c\
 noop.c\
 rename.c\
 resplist.c\
 response.c\
 resproc.c\
 select.c\
 state.c\
 status.c\
 starttls.c\
 store.c\
 storeflg.c\
 subscribe.c\
 tag.c\
 trace.c\
 unselect.c\
 unsubscribe.c\
 folder.c\
 mbox.c\
 url.c
