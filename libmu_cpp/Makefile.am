## This file is part of GNU Mailutils.
## Copyright (C) 2004, 2006-2007, 2009-2012 Free Software Foundation,
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

lib_LTLIBRARIES = libmu_cpp.la

libmu_cpp_la_SOURCES = \
 address.cc\
 attribute.cc\
 body.cc\
 envelope.cc\
 filter.cc\
 folder.cc\
 header.cc\
 iterator.cc\
 list.cc\
 mailbox.cc\
 mailcap.cc\
 mailer.cc\
 message.cc\
 mime.cc\
 mutil.cc\
 pop3.cc\
 registrar.cc\
 secret.cc\
 sieve.cc\
 stream.cc\
 url.cc

libmu_cpp_la_LIBADD = @MU_COMMON_LIBRARIES@
libmu_cpp_la_LDFLAGS = -rpath $(libdir)  -version-info @VI_CURRENT@:@VI_REVISION@:@VI_AGE@

