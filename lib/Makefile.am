AUTOMAKE_OPTIONS = ansi2knr
noinst_LIBRARIES = libmailutils.a

SUBDIRS = posix

INCLUDES = -I${top_srcdir}/include
libmailutils_a_SOURCES = basename.c daemon.c getopt.c getopt1.c md5.c \
                         mu_dbm.c getline.c xstrdup.c xmalloc.c argcv.c

EXTRA_DIST = alloca.c fnmatch.c fgetpwent.c getpass.c malloc.c obstack.c \
  realloc.c setenv.c snprintf.c strchrnul.c strndup.c strnlen.c strncasecmp.c \
  strcasecmp.c strtok_r.c  strsignal.c xstrtol.c  vasprintf.c

noinst_HEADERS = argcv.h error.h fnmatch.h getline.h getopt.h md5.h mu_dbm.h\
 regex.h snprintf.h xalloc.h xstrtol.h obstack.h

libmailutils_a_LIBADD = @LIBOBJS@ @ALLOCA@

