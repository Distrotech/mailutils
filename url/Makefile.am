# Use automake to process this file -*-Makefile-*-

AUTOMAKE_OPTIONS = ../lib/ansi2knr

CFLAGS = -Wall -pedantic -g 

include_HEADERS = url.h

noinst_HEADERS = url0.h

base_sources = _cpystr.c url.c urli.c url_imap.c url_mailto.c \
 url_mbox.c url_pop.c

chewurl_SOURCES = $(base_sources) chewurl.c

EXTRA_DIST = test_url

LDADD = @ALLOCA@ @LIBOBJS@ @INTLLIBS@
datadir = $(prefix)/@DATADIRNAME@
localedir = $(datadir)/locale
INCLUDES = -I../intl -DLOCALEDIR=\"$(localedir)\"
