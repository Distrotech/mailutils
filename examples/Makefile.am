## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2002 Free Software Foundation, Inc.
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
##   Foundation, Inc.
##   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

noinst_PROGRAMS = \
 addr\
 base64\
 listop\
 url-parse\
 sfrom\
 mimetest\
 muemail\
 msg-send\
 http\
 mta\
 decode2047\
 mailcap

moddir=@SIEVE_MODDIR@
mod_LTLIBRARIES = numaddr.la

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib -I$(top_srcdir)/mailbox @INTLINCS@
AM_LDFLAGS = ../mailbox/libmailbox.la ../lib/libmailutils.la

numaddr_la_SOURCES = numaddr.c
numaddr_la_LIBADD = ../libsieve/libsieve.la 
numaddr_la_LDFLAGS = -module -avoid-version -no-undefined 

mimetest_LDADD =\
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/imap/libmu_imap.la\
 ../mailbox/pop/libmu_pop.la\
 ../mailbox/libmailbox.la\
 ../lib/libmailutils.la\
 @AUTHLIBS@ 

mta_LDADD =\
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/libmailbox.la\
 ../lib/libmailutils.la

sfrom_LDADD =\
 ../mailbox/mbox/libmu_mbox.la\
 ../mailbox/imap/libmu_imap.la\
 ../mailbox/pop/libmu_pop.la\
 ../mailbox/libmailbox.la\
 ../lib/libmailutils.la\
 @AUTHLIBS@ 

EXTRA_DIST = comsat.conf dot.biffrc gnu-imap4d.pam gnu-pop3d.pam\
 mailutils.rc reply.scm


