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
##   Foundation, Inc. 51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

CXX_EXAMPLES = \
 addr\
 http\
 iconv\
 listop\
 mailcap\
 murun\
 sfrom\
 url-parse

EXTRA_PROGRAMS=$(CXX_EXAMPLES)
noinst_PROGRAMS = @MU_BUILT_CXX_EXAMPLES@

INCLUDES = @MU_COMMON_INCLUDES@ @INTLINCS@

AM_LDFLAGS = \
 ../../lib/libmuaux.la\
 ../../libmu_cpp/libmu_cpp.la\
 ${MU_LIB_MAILUTILS}\
 @MU_COMMON_LIBRARIES@

addr_SOURCES = addr.cc
http_SOURCES = http.cc
iconv_SOURCES = iconv.cc
listop_SOURCES = listop.cc
mailcap_SOURCES = mailcap.cc
murun_SOURCES = murun.cc
sfrom_SOURCES = sfrom.cc
url_parse_SOURCES = url-parse.cc

LDADD =\
 ../../lib/libmuaux.la\
 ../../libmu_cpp/libmu_cpp.la\
 ${MU_LIB_MBOX}\
 ${MU_LIB_IMAP}\
 ${MU_LIB_POP}\
 ${MU_LIB_NNTP}\
 ${MU_LIB_MH}\
 ${MU_LIB_MAILDIR}\
 ${MU_LIB_AUTH}\
 @MU_AUTHLIBS@\
 ${MU_LIB_MAILUTILS}
