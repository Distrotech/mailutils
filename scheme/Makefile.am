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

bin_SCRIPTS = sieve2scm
EXTRA_SCRIPTS=sieve2scm
# FIXME: Sieve2scm is temporarly exempted from installchecks because
# it may fail starting during checks, if libguile-mailutils-v- library
# has not been previously installed. The proper fix would be to alter
# %load-path during tests.
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT=sieve2scm

sievemoddir=@MU_GUILE_SIEVE_MOD_DIR@

sieve2scm: sieve2scm.scmi sieve.sed
	$(AM_V_GEN)sed -f sieve.sed $(srcdir)/sieve2scm.scmi > sieve2scm
	$(AM_V_at)chmod +w sieve2scm

sieve.sed: Makefile
	$(AM_V_GEN)echo 's,%GUILE_BINDIR%,@GUILE_BINDIR@,g' > sieve.sed
	$(AM_V_at)echo 's,%BINDIR%,$(bindir),g' >> sieve.sed
	$(AM_V_at)echo 's,%GUILE_SITE%,$(GUILE_SITE),g' >> sieve.sed
	$(AM_V_at)echo 's,%LIBDIR%,$(sievemoddir),g' >> sieve.sed
	$(AM_V_at)echo 's,%PACKAGE%,$(PACKAGE),g' >> sieve.sed
	$(AM_V_at)echo 's,%VERSION%,$(VERSION),g' >> sieve.sed

CLEANFILES = sieve2scm sieve.sed

sitedir=@GUILE_SITE@/$(PACKAGE)
site_DATA=sieve-core.scm

sievemod_DATA=\
 mimeheader.scm\
 numaddr.scm\
 redirect.scm\
 reject.scm\
 vacation.scm

EXTRA_DIST=\
 $(sievemod_DATA)\
 sieve-core.scm\
 sieve2scm.scmi

