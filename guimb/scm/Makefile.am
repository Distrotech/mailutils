bin_SCRIPTS = sieve.scm

sieve.scm: sieve.scm.in
	sed "s,%BINDIR%,$(bindir),g;s,%LIBDIR%,$(pkgdatadir),g" $< > $@

CLEANFILES = sieve.scm

pkgdata_DATA=\
 mimeheader.scm\
 numaddr.scm\
 redirect.scm\
 reject.scm\
 sieve-core.scm

EXTRA_DIST=\
 mimeheader.scm\
 numaddr.scm\
 redirect.scm\
 reject.scm\
 sieve-core.scm\
 sieve.scm.in

