AUTOMAKE_OPTIONS = ansi2knr
noinst_LTLIBRARIES = libmailutils.la

SUBDIRS = posix

INCLUDES = -I${top_srcdir}/include
libmailutils_la_SOURCES = basename.c daemon.c getopt.c getopt1.c md5.c \
                         mu_dbm.c getline.c xstrdup.c xmalloc.c argcv.c \
                         pin.c mu_asprintf.c

EXTRA_DIST = alloca.c fnmatch.c fgetpwent.c getpass.c malloc.c obstack.c \
  realloc.c setenv.c snprintf.c strchrnul.c strndup.c strnlen.c strncasecmp.c \
  strcasecmp.c strtok_r.c  strsignal.c xstrtol.c  vasprintf.c \
  argp-ba.c argp-eexst.c argp-fmtstream.c argp-fs-xinl.c \
  argp-help.c argp-parse.c argp-pv.c argp-pvh.c argp-xinl.c \
  utmp.c 

noinst_HEADERS = argcv.h error.h fnmatch.h getline.h getopt.h md5.h mu_dbm.h\
 regex.h snprintf.h xalloc.h xstrtol.h obstack.h \
 argp-fmtstream.h argp-namefrob.h argp.h mu_asprintf.h

libmailutils_la_LIBADD = @LIBOBJS@ @ALLOCA@

