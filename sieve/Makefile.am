#CFLAGS = -Wall -pedantic -g -DTESTING
AUTOMAKE_OPTIONS = ../lib/ansi2knr
INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib @ARGPINCS@
YLWRAP = $(SHELL) $(srcdir)/ylwrap
YFLAGS = -d

BUILT_SOURCES= \
  sieve-gram.c sieve-gram.h \
  sieve-lex.c addr-lex.c \
  addr.c addr.h

MOSTLYCLEANFILES=
CLEANFILES=
DISTCLEANFILES=
MAINTAINERCLEANFILES=$(BUILT_SOURCES)
EXTRA_DIST = \
  $(BUILT_SOURCES) \
  addr-lex.l addr.y gram-sieve.y sieve-gram.y sieve-lex.l md5-rsa.h

bin_PROGRAMS = sieve

SRC = \
  addr-lex.c \
  addr.c \
  comparator.c \
  imparse.c \
  interp.c \
  message.c \
  md5-rsa.c \
  parseaddr.c \
  script.c \
  sieve-lex.c \
  sieve-gram.c \
  sieve_err.c \
  svcb.c \
  svctx.c \
  svfield.c \
  svutil.c \
  tree.c \
  util.c

HDR = \
  comparator.h \
  imparse.h \
  interp.h \
  message.h \
  parseaddr.h \
  script.h \
  sieve_err.h \
  sieve_interface.h \
  svfield.h \
  sv.h \
  tree.h \
  util.h

sieve_DEPENDENCIES = ../mailbox/libmailbox.la
sieve_LDADD = @ARGPLIBS@ ../mailbox/libmailbox.la ../lib/libmailutils.a
sieve_SOURCES = \
  sieve.c \
  ${SRC} $(HDR)

addr-lex.c: $(srcdir)/addr-lex.l addr.h
	$(YLWRAP) "$(LEX)" $(srcdir)/addr-lex.l lex.yy.c \
                  addr-lex.c -- -yy addr

addr.c addr.h: $(srcdir)/addr.y
	$(YLWRAP) "$(YACC) $(YFLAGS)" $(srcdir)/addr.y \
                  y.tab.c addr.c y.tab.h addr.h \
                  -- -yy addr

sieve-lex.c: $(srcdir)/sieve-lex.l sieve-gram.h
	$(YLWRAP) "$(LEX)" $(srcdir)/sieve-lex.l lex.yy.c sieve-lex.c

sieve-gram.c sieve-gram.h: $(srcdir)/sieve-gram.y
	$(YLWRAP) "$(YACC) $(YFLAGS)" $(srcdir)/sieve-gram.y \
	          y.tab.c sieve-gram.c y.tab.h sieve-gram.h

comparator.o: sieve-gram.h
script.o: sieve-gram.h
tree.o: sieve-gram.h

empty: clean
	rm -f $(BUILT_SOURCES)

