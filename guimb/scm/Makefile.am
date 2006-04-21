## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2001, 2002, 2006 Free Software Foundation, Inc.
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

bin_SCRIPTS = sieve.scm

sievemoddir=$(GUILE_SITE)/sieve-modules

sieve.scm: sieve.scm.in sieve.sed
	sed -f sieve.sed $(srcdir)/sieve.scm.in > sieve.scm

sieve.sed: Makefile
	echo 's,%GUILE_BINDIR%,@GUILE_BINDIR@,g' > sieve.sed
	echo 's,%BINDIR%,$(bindir),g' >> sieve.sed
	echo 's,%GUILE_SITE%,$(GUILE_SITE),g' >> sieve.sed
	echo 's,%LIBDIR%,$(sievemoddir),g' >> sieve.sed
	echo 's,%PACKAGE%,$(PACKAGE),g' >> sieve.sed
	echo 's,%VERSION%,$(VERSION),g' >> sieve.sed

CLEANFILES = sieve.scm sieve.sed

sitedir=$(GUILE_SITE)/$(PACKAGE)
GUILE_SITE_DATA_X=sieve-core.scm
site_DATA=@GUILE_SITE_DATA@

sievemod_DATA=\
 mimeheader.scm\
 numaddr.scm\
 redirect.scm\
 reject.scm\
 vacation.scm

EXTRA_DIST=\
 mimeheader.scm\
 numaddr.scm\
 redirect.scm\
 reject.scm\
 sieve-core.scm\
 sieve.scm.in\
 vacation.scm

