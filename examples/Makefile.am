## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
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

SUBDIRS = config cpp scheme

noinst_PROGRAMS = \
 addr\
 argcv\
 base64\
 decode2047\
 encode2047\
 http\
 iconv\
 listop\
 lsf\
 mailcap\
 mimetest\
 msg-send\
 mta\
 muemail\
 murun\
 nntpclient\
 pop3client \
 sfrom\
 url-parse

## NOTE: Numaddr must be an installable target, otherwise libtool
## will not create a shared library and `make check' will fail in
## sieve. Pity.
## -Sergey
moddir=@SIEVE_MODDIR@
mod_LTLIBRARIES = numaddr.la

INCLUDES = @MU_COMMON_INCLUDES@ @INTLINCS@

AM_LDFLAGS = ../lib/libmuaux.la ../mailbox/libmailutils.la @MU_COMMON_LIBRARIES@

argcv_LDADD =\
 ../lib/libmuaux.la\
 ../mailbox/libmailutils.la

numaddr_la_SOURCES = numaddr.c
numaddr_la_LIBADD = ../libsieve/libsieve.la
numaddr_la_LDFLAGS = -module -avoid-version -no-undefined

mimetest_LDADD =\
 ../lib/libmuaux.la\
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/imap/libmu_imap.la\
 ../mailbox/pop/libmu_pop.la\
 @MU_AUTHLIBS@\
 ../mailbox/libmailutils.la

mta_LDADD =\
 ../lib/libmuaux.la\
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/libmailutils.la

sfrom_LDADD =\
 ../lib/libmuaux.la\
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/imap/libmu_imap.la\
 ../mailbox/pop/libmu_pop.la\
 ../mailbox/nntp/libmu_nntp.la\
 ../mailbox/mh/libmu_mh.la\
 ../mailbox/maildir/libmu_maildir.la\
 @MU_AUTHLIBS@\
 ../mailbox/libmailutils.la

pop3client_LDADD = \
 ../lib/libmuaux.la\
 ../mailbox/pop/libmu_pop.la\
 @MU_AUTHLIBS@\
 ../mailbox/libmailutils.la\
 @READLINE_LIBS@

nntpclient_LDADD = \
 ../lib/libmuaux.la\
 ../mailbox/nntp/libmu_nntp.la\
 @MU_AUTHLIBS@\
 ../mailbox/libmailutils.la\
 @READLINE_LIBS@

lsf_LDADD = \
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/imap/libmu_imap.la\
 ../mailbox/pop/libmu_pop.la\
 ../mailbox/nntp/libmu_nntp.la\
 ../mailbox/mh/libmu_mh.la\
 ../mailbox/maildir/libmu_maildir.la\
 @MU_AUTHLIBS@ \
 ../mailbox/libmailutils.la

