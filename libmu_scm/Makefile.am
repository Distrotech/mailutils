## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2001, 2002 Free Software Foundation, Inc.
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
##   Foundation, Inc.
##   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

INCLUDES =-I. -I$(top_builddir) -I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/include @GUILE_INCLUDES@ @INTLINCS@

lib_LTLIBRARIES=@MU_SCM@
EXTRA_LTLIBRARIES=libmu_scm.la
libmu_scm_la_SOURCES=\
 mu_address.c\
 mu_body.c\
 mu_guimb.c\
 mu_mailbox.c\
 mu_message.c\
 mu_mime.c\
 mu_logger.c\
 mu_port.c\
 mu_scm.c\
 mu_util.c\
 mu_scm.h

libmu_scm_la_LDFLAGS = -rpath $(libdir) -version-info 0:0:0

EXTRA_DIST=mailutils.scm mailutils.scm.in

pkgdata_DATA=@GUILE_PKGDATA@
DOT_X_FILES=mu_address.x mu_body.x mu_guimb.x mu_mailbox.x mu_message.x mu_mime.x\
 mu_logger.x mu_port.x mu_scm.x mu_util.x
DOT_DOC_FILES=mu_address.doc mu_body.doc mu_guimb.doc mu_mailbox.doc mu_message.doc \
 mu_mime.doc mu_logger.doc mu_port.doc mu_scm.doc mu_util.doc 
BUILT_SOURCES=@GUILE_BUILT_SOURCES@ 

DISTCLEANFILES=$(DOT_X_FILES) $(DOT_DOC_FILES) @GUILE_PKGDATA@
CLEANFILES=*.inc

AM_CPPFLAGS=-DDATADIR=\"$(pkgdatadir)\"
ETAGS_ARGS = --regex='/SCM_\(GLOBAL_\)?\(G?PROC\|G?PROC1\|SYMBOL\|VCELL\|CONST_LONG\).*\"\([^\"]\)*\"/\3/' \
   --regex='/[ \t]*SCM_[G]?DEFINE1?[ \t]*(\([^,]*\),[^,]*/\1/'

GUILE_DOC_SNARF=$(top_srcdir)/scripts/guile-$(GUILE_SNARF_VERSION)/guile-doc-snarf

SUFFIXES=.x .doc .inc .in

.c.x:
	AWK=$(AWK) \
	$(GUILE_DOC_SNARF) -o $@ \
                           $< $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) 

.c.doc:
	AWK=$(AWK) \
	$(GUILE_DOC_SNARF) -d -o $@ \
                           $< $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) 

.c.inc:
	sed -n 's/SCM_DEFINE *(.[^,]*, *\"\([^"][^"]*\)\".*/[(export \1)]/p' $< > $@

guile-procedures.txt: $(DOT_DOC_FILES)
	cat $(DOT_DOC_FILES) > $@

mailutils.scm: mailutils.scm.in $(libmu_scm_la_SOURCES:.c=.inc)

.in:
	m4 -DVERSION=$(VERSION) -DLIBDIR=$(libdir) \
           -DBUILDDIR=$(top_builddir)/libmu_scm $< > $@


## Add -MG to make the .x magic work with auto-dep code.
MKDEP = $(CC) -M -MG $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)

install-data-hook:
	@here=`pwd`; \
	 cd $(DESTDIR)$(libdir);\
	 if test -f libmu_scm.so; then \
		$(LN_S) -f libmu_scm.so libguile-mailutils-v-$(VERSION).so; \
	 fi; \
	 cd $$here
