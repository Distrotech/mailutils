noinst_LIBRARIES = libmailutils.a

libmailutils_a_SOURCES = basename.c getopt.c getopt1.c md5.c getline.c \
  xstrdup.c xmalloc.c argcv.c

EXTRA_DIST = alloca.c snprintf.c strtok_r.c xstrtol.c

noinst_HEADERS = getopt.h md5.h getline.h snprintf.h xstrtol.h xalloc.h \
 argcv.h error.h

libmailutils_a_LIBADD = @LIBOBJS@ @ALLOCA@

CFLAGS = -Wall -pedantic -g
