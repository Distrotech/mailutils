AUTOMAKE_OPTIONS = ../lib/ansi2knr
INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/include \
 -I$(top_srcdir)/libmu_scm

SUBDIRS=scm

bin_PROGRAMS=@GUIMB@
EXTRA_PROGRAMS=guimb
guimb_SOURCES=main.c collect.c util.c run_scm.c guimb.h 
guimb_LDADD = ../libmu_scm/libmu_scm.a ../mailbox/libmailbox.la \
 ../lib/libmailutils.a @GUILE_LIBS@ 

pkgdata_DATA=@GUILE_PROCEDURES_TXT@
DOT_X_FILES=run_scm.x
DOT_DOC_FILES=run_scm.doc
BUILT_SOURCES=@GUILE_BUILT_SOURCES@

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

if MAINTAINER_MODE

## Add -MG to make the .x magic work with auto-dep code.
MKDEP = $(CC) -M -MG $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)

endif
