##   Process this file with GNU Automake to create Makefile.in
##
##   Copyright (C) 2004, 2006 Free Software Foundation, Inc.
##
##   GNU Mailutils is free software; you can redistribute it and/or
##   modify it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 2, or (at
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

INCLUDES = @MU_COMMON_INCLUDES@ @INTLINCS@

MU_CXX_LIBS = libmu_cpp.la
lib_LTLIBRARIES = @MU_CXX_LIBS@
EXTRA_LTLIBRARIES = $(MU_CXX_LIBS)

libmu_cpp_la_SOURCES = \
 address.cc\
 filter.cc\
 header.cc\
 iterator.cc\
 list.cc\
 mailbox.cc\
 mailcap.cc\
 mailer.cc\
 message.cc\
 pop3.cc\
 stream.cc\
 url.cc

libmu_cpp_la_LIBADD = @MU_COMMON_LIBRARIES@
libmu_cpp_la_LDFLAGS = -version-info @VI_CURRENT@:@VI_REVISION@:@VI_AGE@

