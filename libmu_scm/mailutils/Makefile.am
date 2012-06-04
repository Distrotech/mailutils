## This file is part of GNU Mailutils.
## Copyright (C) 2001-2002, 2006-2007, 2009-2012 Free Software
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

EXTRA_DIST=mailutils.scm.in

CLEANFILES=\
 mailutils.scm

sitedir   = @GUILE_SITE@/$(PACKAGE)
site_DATA = mailutils.scm

MU_FEATURES=\
 -DVERSION=$(VERSION)\
 -DLIBDIR=$(libdir)\
 -DSITEDIR=$(sitedir)\
 -DBUILDDIR=$(top_builddir)/libmu_scm

if MU_COND_SUPPORT_POP
  MU_FEATURES += -DMU_SUPPORT_POP
endif

if MU_COND_SUPPORT_IMAP
  MU_FEATURES += -DMU_SUPPORT_IMAP
endif

if MU_COND_SUPPORT_NNTP
  MU_FEATURES += -DMU_SUPPORT_NNTP
endif

if MU_COND_SUPPORT_MH
  MU_FEATURES += -DMU_SUPPORT_MH
endif

if MU_COND_SUPPORT_MAILDIR
  MU_FEATURES += -DMU_SUPPORT_MAILDIR
endif

mailutils.scm: mailutils.scm.in
	$(AM_V_GEN)m4 $(MU_FEATURES) $(srcdir)/mailutils.scm.in > $@

