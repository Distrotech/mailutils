## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2002, 2003 Free Software Foundation, Inc.
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
##   Foundation, Inc.
##   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/mailbox/include -I${top_srcdir}/lib @INTLINCS@
AM_CFLAGS = -DSITE_VIRTUAL_PWDDIR=\"@SITE_VIRTUAL_PWDDIR@\" \
            -DSITE_CRAM_MD5_PWD=\"@SITE_CRAM_MD5_PWD@\"

lib_LTLIBRARIES = libmuauth.la

libmuauth_la_SOURCES = \
gsasl.c \
pam.c \
sql.c \
tls.c \
virtual.c 

libmuauth_la_LIBADD = @LTLIBINTL@ 
libmuauth_la_LDFLAGS = -version-info 0:0:0

