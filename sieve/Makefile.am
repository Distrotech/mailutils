CFLAGS = -Wall -pedantic -g -DTESTING
INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib

MOSTLYCLEANFILES=
CLEANFILES=
DISTCLEANFILES=
MAINTAINERCLEANFILES=\
  sieve-gram.c sieve-gram.h \
  sieve-lex.c \
  addr-lex.c \
  addr.c addr.h

bin_PROGRAMS = test sieve

SRC = \
  addr-lex.c \
  addr.c \
  comparator.c \
  imparse.c \
  interp.c \
  md5.c \
  message.c \
  parseaddr.c \
  script.c \
  sieve-lex.c \
  sieve-gram.c \
  sieve_err.c \
  svfield.c \
  tree.c \
  util.c \
  xmalloc.c

test_DEPENDENCIES =
test_LDADD =
test_SOURCES = \
  test.c \
  ${SRC}


sieve_DEPENDENCIES = ../mailbox/libmailbox.la
sieve_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.a
sieve_SOURCES = \
  sieve.c \
  ${SRC}

YACC = bison -y
YFLAGS = -d
LEX = flex

addr-lex.c: addr-lex.l addr.h
	$(LEX) -t -Paddr addr-lex.l > $@

addr.c addr.h: addr.y
	$(YACC) $(YFLAGS) -p addr addr.y
	mv -f y.tab.c addr.c
	mv -f y.tab.h addr.h

sieve-lex.c: sieve-lex.l sieve-gram.h
	$(LEX) -t sieve-lex.l > $@

sieve-gram.c sieve-gram.h: sieve-gram.y
	$(YACC) $(YFLAGS) sieve-gram.y
	mv -f y.tab.c sieve-gram.c
	mv -f y.tab.h sieve-gram.h

comparator.o: sieve-gram.h
script.o: sieve-gram.h
tree.o: sieve-gram.h

