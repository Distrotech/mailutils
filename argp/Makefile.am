AUTOMAKE_OPTIONS = ../lib/ansi2knr

INCLUDES = -I${top_srcdir}/lib

noinst_LTLIBRARIES =  @ARGPLIB@

libargp_la_SOURCES = argp-ba.c argp-eexst.c argp-fmtstream.c argp-fs-xinl.c \
 argp-help.c argp-parse.c argp-pv.c argp-pvh.c argp-xinl.c pin.c

EXTRA_DIST =

EXTRA_LTLIBRARIES = libargp.la

noinst_HEADERS = argp-fmtstream.h argp-namefrob.h argp.h

# Feb 20, 2002 - JB Consider putting this back in when QNX has
# a recent libtool
# libargp_la_LIBADD = ../lib/libmailutils.la

