# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

EXTRA_DIST = makespool
test_dirs = lib etc spool folder

dist-hook:
	here=`cd $(top_builddir)/$(subdir) && pwd`; \
	srcdir=`cd $(srcdir) && pwd`; \
	distdir=`cd $(distdir) && pwd`; \
	for dir in $(test_dirs); \
	do \
            cd $$srcdir;\
            mkdir $$distdir/$$dir;\
	    cd $$dir;\
	    for file in DISTFILES `cat DISTFILES`; do \
	        d=$$srcdir/$$dir; \
	        if test -d $$d/$$file; then \
                    cp -pr $$d/$$file $$distdir/$$dir/$$file; \
		else \
	            test -f $$distdir/$$dir/$$file \
                        || cp -p $$d/$$file $$distdir/$$dir/$$file || exit; \
                fi; \
	    done;\
        done;\
	cd $$here

DISTCLEANFILES=*.exp *.log *.sum 

distclean-local:
	-rm -rf	data
