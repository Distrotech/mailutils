AUTOMAKE_OPTIONS = ansi2knr
noinst_LIBRARIES = libmailutils.a

SUBDIRS = posix

libmailutils_a_SOURCES = basename.c getopt.c getopt1.c md5.c getline.c \
  xstrdup.c xmalloc.c argcv.c

EXTRA_DIST = alloca.c fnmatch.c setenv.c snprintf.c strchrnul.c strndup.c \
  strnlen.c strtok_r.c strsignal.c xstrtol.c vasprintf.c malloc.c realloc.c \
  fgetpwent.c

noinst_HEADERS = argcv.h error.h fnmatch.h getline.h getopt.h md5.h \
 regex.h snprintf.h xalloc.h xstrtol.h

libmailutils_a_LIBADD = @LIBOBJS@ @ALLOCA@

