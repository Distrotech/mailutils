## This file is part of GNU Mailutils.
## Copyright (C) 2001-2003, 2007, 2009-2012 Free Software Foundation,
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

mhlibdir   = $(pkgdatadir)/mh

LISPSRC    = mailutils-mh.el
lisp_LISP  = @lisp_LISP@
EXTRA_LISP = mailutils-mh.el

MH_FILES   = \
 components\
 forwcomps\
 mhl.format\
 mhl.forward\
 mhl.headers\
 mhl.repl\
 mhl.usenet\
 replcomps\
 replgroupcomps\
 scan.default\
 scan.mailx\
 scan.size\
 scan.time\
 scan.timely

mhlib_DATA = $(MH_FILES) $(LISPSRC)

EXTRA_DIST = $(MH_FILES) mailutils-mh.eli
DISTCLEANFILES = mailutils-mh.el

SUFFIXES = .eli .el
mailutils-mh.el: mailutils-mh.eli

.eli.el:
	sed "s,BINDIR,$(bindir),g;s,MHLIBDIR,$(mhlibdir),g" $< > $@

