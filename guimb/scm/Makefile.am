# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

bin_SCRIPTS = sieve.scm

sieve.scm: sieve.scm.in
	sed "s,%BINDIR%,$(bindir),g;s,%LIBDIR%,$(pkgdatadir),g;s,%PACKAGE%,$(PACKAGE),g;s,%VERSION%,$(VERSION),g" $< > $@

CLEANFILES = sieve.scm

pkgdata_DATA=\
 mimeheader.scm\
 numaddr.scm\
 redirect.scm\
 reject.scm\
 sieve-core.scm\
 vacation.scm

EXTRA_DIST=\
 mimeheader.scm\
 numaddr.scm\
 redirect.scm\
 reject.scm\
 sieve-core.scm\
 sieve.scm.in\
 vacation.scm

