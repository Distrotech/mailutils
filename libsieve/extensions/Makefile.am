## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2004, 2005, 2006 Free Software Foundation, Inc.
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

moddir=@SIEVE_MODDIR@
mod_LTLIBRARIES = \
 list.la\
 moderator.la\
 spamd.la\
 timestamp.la\
 vacation.la

INCLUDES = @MU_COMMON_INCLUDES@ @INTLINCS@

AM_LDFLAGS = ../../lib/libmuaux.la ${MU_LIB_MAILUTILS} 

list_la_SOURCES = list.c
list_la_LIBADD = ../libsieve.la 
list_la_LDFLAGS = -module -avoid-version -no-undefined 

moderator_la_SOURCES = moderator.c
moderator_la_LIBADD = ../libsieve.la 
moderator_la_LDFLAGS = -module -avoid-version -no-undefined 

spamd_la_SOURCES = spamd.c
spamd_la_LIBADD = ../libsieve.la 
spamd_la_LDFLAGS = -module -avoid-version -no-undefined 

timestamp_la_SOURCES = timestamp.c
timestamp_la_LIBADD = ../libsieve.la 
timestamp_la_LDFLAGS = -module -avoid-version -no-undefined 

vacation_la_SOURCES = vacation.c
vacation_la_LIBADD = ../libsieve.la 
vacation_la_LDFLAGS = -module -avoid-version -no-undefined 

