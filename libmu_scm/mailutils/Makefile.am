## This file is part of GNU Mailutils.
## Copyright (C) 2001, 2002, 2006, 2007, 2009, 2010, 2011 Free Software
## Foundation, Inc.
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

EXTRA_DIST=mailutils.scm mailutils.scm.in

DISTCLEANFILES=\
 mailutils.scm

sitedir   = @GUILE_SITE@/$(PACKAGE)
site_DATA = mailutils.scm

mailutils.scm: mailutils.scm.in
	$(AM_V_GEN)m4 -DVERSION=$(VERSION) -DLIBDIR=$(libdir) \
	   -DSITEDIR=$(sitedir) \
           -DBUILDDIR=$(top_builddir)/libmu_scm \
            $(srcdir)/mailutils.scm.in > $@

