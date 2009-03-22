## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2001, 2002, 2006, 2007,
##   2009 Free Software Foundation, Inc.
##
##   GNU Mailutils is free software; you can redistribute it and/or
##   modify it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 3, or (at
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

INCLUDES = -I. @MU_COMMON_INCLUDES@ @GUILE_INCLUDES@ 

MU_SCM_LTLIBRARIES_LIST = libmu_scm.la
lib_LTLIBRARIES=@MU_SCM_LTLIBRARIES_BUILD@
EXTRA_LTLIBRARIES=libmu_scm.la
C_SRCS=\
 mu_address.c\
 mu_body.c\
 mu_guimb.c\
 mu_mailbox.c\
 mu_message.c\
 mu_mime.c\
 mu_logger.c\
 mu_port.c\
 mu_scm.c\
 mu_util.c

libmu_scm_la_SOURCES=\
 $(C_SRCS) \
 mu_scm.h

libmu_scm_la_LDFLAGS = -rpath $(libdir) -version-info @VI_CURRENT@:@VI_REVISION@:@VI_AGE@
libmu_scm_la_LIBADD = \
 ${MU_LIB_MBOX}\
 ${MU_LIB_IMAP}\
 ${MU_LIB_POP}\
 ${MU_LIB_NNTP}\
 ${MU_LIB_MH}\
 ${MU_LIB_MAILDIR}\
 ${MU_LIB_MAILER}\
 ${MU_LIB_MAILUTILS}\
 @GUILE_LIBS@

EXTRA_DIST=mailutils.scm mailutils.scm.in

sitedir=@MU_GUILE_SITE_DIR@
GUILE_SITE_DATA_X=guile-procedures.txt mailutils.scm
site_DATA=@GUILE_SITE_DATA@

DOT_X_FILES=\
 mu_address.x\
 mu_body.x\
 mu_guimb.x\
 mu_mailbox.x\
 mu_message.x\
 mu_mime.x\
 mu_logger.x\
 mu_port.x\
 mu_scm.x\
 mu_util.x

DOT_DOC_FILES=\
 mu_address.doc\
 mu_body.doc\
 mu_guimb.doc\
 mu_mailbox.doc\
 mu_message.doc\
 mu_mime.doc\
 mu_logger.doc\
 mu_port.doc\
 mu_scm.doc\
 mu_util.doc 

GUILE_BUILT_SOURCES_X=$(DOT_X_FILES) $(DOT_DOC_FILES) guile-procedures.texi

BUILT_SOURCES=@GUILE_BUILT_SOURCES@ 

DISTCLEANFILES=$(DOT_X_FILES) $(DOT_DOC_FILES) guile-procedures.texi @GUILE_SITE_DATA@
CLEANFILES=*.inc

AM_CPPFLAGS=-DDATADIR=\"$(pkgdatadir)\"
ETAGS_ARGS = --regex='/SCM_\(GLOBAL_\)?\(G?PROC\|G?PROC1\|SYMBOL\|VCELL\|CONST_LONG\).*\"\([^\"]\)*\"/\3/' \
   --regex='/[ \t]*SCM_[G]?DEFINE1?[ \t]*(\([^,]*\),[^,]*/\1/'

GUILE_DOC_SNARF=$(mu_aux_dir)/guile-doc-snarf

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

guile-procedures.texi: $(DOT_DOC_FILES)
	cat $(DOT_DOC_FILES) > $@

guile-procedures.txt: guile-procedures.texi
	rm -f $@
	$(MAKEINFO) --force -o $@ guile-procedures.texi || test -f $@

mailutils.scm: mailutils.scm.in $(C_SRCS:.c=.inc)
	m4 -DVERSION=$(VERSION) -DLIBDIR=$(libdir) \
	   -DSITEDIR=$(sitedir) \
           -DBUILDDIR=$(top_builddir)/libmu_scm \
            $(srcdir)/mailutils.scm.in > $@


## Add -MG to make the .x magic work with auto-dep code.
MKDEP = $(CC) -M -MG $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)

install-data-hook:
	@here=`pwd`; \
	 cd $(DESTDIR)$(libdir);\
	 if test -f libmu_scm.so; then \
		$(LN_S) -f libmu_scm.so libguile-mailutils-v-$(VERSION).so; \
	 fi; \
	 cd $$here

CFLOW_FLAGS=-i^s --brief -v
libmu_scm_CFLOW_INPUT=$(libmu_scm_la_OBJECTS:.lo=.c)
libmu_scm.cflow: $(libmu_scm_CFLOW_INPUT) Makefile
	cflow -o$@ $(CFLOW_FLAGS) $(DEFS) \
		$(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
		$(CPPFLAGS) \
	$(libmu_scm_CFLOW_INPUT)
