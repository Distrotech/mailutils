AUTOMAKE_OPTIONS = ../lib/ansi2knr

INCLUDES = -I${top_srcdir}/lib

noinst_LIBRARIES =  @ARGPLIB@

libargp_a_SOURCES = argp-ba.c argp-eexst.c argp-fmtstream.c argp-fs-xinl.c \
 argp-help.c argp-parse.c argp-pv.c argp-pvh.c argp-xinl.c pin.c

EXTRA_DIST =

EXTRA_LIBRARIES = libargp.a

noinst_HEADERS = argp-fmtstream.h argp-namefrob.h argp.h

libargp_a_LIBADD = ../lib/libmailutils.a

