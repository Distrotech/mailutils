bin_PROGRAMS = scan inc
noinst_LIBRARIES = libmh.a
libmh_a_SOURCES= \
 mh_argp.c\
 mh_getopt.c\
 mh_error.c\
 mh_format.c\
 mh_init.c\
 mh_fmtgram.y
noinst_HEADERS = mh.h mh_getopt.h
scan_SOURCES = scan.c
inc_SOURCES = inc.c
INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib @ARGPINCS@
mh_LIBS = ./libmh.a @ARGPLIBS@ ../mailbox/libmailbox.la ../lib/libmailutils.la
scan_LDADD = $(mh_LIBS)  -lcurses -ltermcap
inc_LDADD = $(mh_LIBS)
YFLAGS=-vt

