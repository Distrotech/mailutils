## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2002, 2003, 2005, 2006 Free Software Foundation, Inc.
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
##   Foundation, Inc.  51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

INCLUDES = @MU_COMMON_INCLUDES@ @INTLINCS@
AM_CFLAGS = -DSITE_VIRTUAL_PWDDIR=\"@SITE_VIRTUAL_PWDDIR@\" \
            -DSITE_CRAM_MD5_PWD=\"@SITE_CRAM_MD5_PWD@\"

lib_LTLIBRARIES = libmuauth.la

libmuauth_la_SOURCES = \
 gsasl.c\
 lbuf.c\
 lbuf.h\
 pam.c\
 radius.c\
 sql.c\
 sql.h\
 tls.c\
 virtual.c 

libmuauth_la_LIBADD = ${MU_LIB_MAILUTILS} @MU_AUTHLIBS@ @SQLLIB@ @LTLIBINTL@
libmuauth_la_LDFLAGS = -version-info @VI_CURRENT@:@VI_REVISION@:@VI_AGE@
libmuauth_la_DEPENDENCIES = @SQLLIB_DEPENDENCY@ 
