# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

AUTOMAKE_OPTIONS = ../lib/ansi2knr
INCLUDES =-I. -I$(top_builddir) -I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/include

noinst_LIBRARIES=@MU_SCM@
EXTRA_LIBRARIES=libmu_scm.a
libmu_scm_a_SOURCES=\
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

pkgdata_DATA=@GUILE_PROCEDURES_TXT@
DOT_X_FILES=mu_address.x mu_body.x mu_guimb.x mu_mailbox.x mu_message.x mu_mime.x\
 mu_logger.x mu_port.x mu_scm.x mu_util.x
DOT_DOC_FILES=mu_address.doc mu_body.doc mu_guimb.doc mu_mailbox.doc mu_message.doc \
 mu_mime.doc mu_logger.doc mu_port.doc mu_scm.doc mu_util.doc 
BUILT_SOURCES=@GUILE_BUILT_SOURCES@ 

DISTCLEANFILES=$(DOT_X_FILES) $(DOT_DOC_FILES) @GUILE_PROCEDURES_TXT@

CPPFLAGS=-DDATADIR=\"$(pkgdatadir)\"
ETAGS_ARGS = --regex='/SCM_\(GLOBAL_\)?\(G?PROC\|G?PROC1\|SYMBOL\|VCELL\|CONST_LONG\).*\"\([^\"]\)*\"/\3/' \
   --regex='/[ \t]*SCM_[G]?DEFINE1?[ \t]*(\([^,]*\),[^,]*/\1/'

GUILE_DOC_SNARF=$(top_srcdir)/scripts/guile-doc-snarf
SUFFIXES=.x .doc

.c.x:
	$(GUILE_DOC_SNARF) $< $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) \
                           $< > $@ || { rm $@; false; }

.c.doc:
	$(GUILE_DOC_SNARF) $< $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) \
                           $< > /dev/null || { rm $@; false; }

guile-procedures.txt: $(DOT_DOC_FILES)
	cat $(DOT_DOC_FILES) > $@

## Add -MG to make the .x magic work with auto-dep code.
MKDEP = $(CC) -M -MG $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
