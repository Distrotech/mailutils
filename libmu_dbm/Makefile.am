## This file is part of GNU Mailutils.
## Copyright (C) 2011-2012 Free Software Foundation, Inc.
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

INCLUDES = @MU_LIB_COMMON_INCLUDES@ @DBMINCLUDES@

lib_LTLIBRARIES = libmu_dbm.la

libmu_dbm_la_SOURCES = \
 close.c\
 create.c\
 datumfree.c\
 dbm.c\
 delete.c\
 destroy.c\
 errstr.c\
 fetch.c\
 firstkey.c\
 getfd.c\
 name.c\
 nextkey.c\
 open.c\
 safety.c\
 store.c\
 berkeley.c\
 gdbm.c\
 kyoto.c\
 ndbm.c\
 tokyo.c

noinst_HEADERS = mudbm.h

libmu_dbm_la_LIBADD = ${MU_LIB_MAILUTILS} @MU_AUTHLIBS@ @DBMLIBS@ @LTLIBINTL@
libmu_dbm_la_LDFLAGS = -version-info @VI_CURRENT@:@VI_REVISION@:@VI_AGE@
libmu_dbm_la_DEPENDENCIES = @DBMLIB_DEPENDENCY@ 
