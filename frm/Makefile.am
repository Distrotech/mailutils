## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2001, 2002 Free Software Foundation, Inc.
##
##   GNU Mailtuils is free software; you can redistribute it and/or
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

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib @INTLINCS@
SUBDIRS = testsuite

bin_PROGRAMS = frm
frm_SOURCES = frm.c
frm_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.la @LTLIBINTL@
