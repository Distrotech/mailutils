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

bin_SCRIPTS = sieve2scm guimb

# FIXME: Sieve2scm is temporarly exempted from installchecks because
# it may fail starting during checks, if libguile-mailutils-v- library
# has not been previously installed. The proper fix would be to alter
# %load-path during tests.
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT=sieve2scm

sievemoddir=@MU_GUILE_SIEVE_MOD_DIR@

sieve2scm: sieve2scm.scmi package.sed
	$(AM_V_GEN)sed -f package.sed $(srcdir)/sieve2scm.scmi > sieve2scm
	$(AM_V_at)chmod +w sieve2scm

guimb: guimb.scmi package.sed
	$(AM_V_GEN)sed -f package.sed $(srcdir)/guimb.scmi > guimb
	$(AM_V_at)chmod +w guimb

package.sed: Makefile
	$(AM_V_GEN)echo 's,%GUILE_BINDIR%,@GUILE_BINDIR@,g' > package.sed
	$(AM_V_at)echo 's,%BINDIR%,$(bindir),g' >> package.sed
	$(AM_V_at)echo 's,%GUILE_SITE%,$(GUILE_SITE),g' >> package.sed
	$(AM_V_at)echo 's,%LIBDIR%,$(sievemoddir),g' >> package.sed
	$(AM_V_at)echo 's,%PACKAGE%,$(PACKAGE),g' >> package.sed
	$(AM_V_at)echo 's,%VERSION%,$(VERSION),g' >> package.sed

CLEANFILES = sieve2scm guimb package.sed

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
 sieve2scm.scmi\
 guimb.scmi

