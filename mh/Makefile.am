bin_PROGRAMS = scan
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
INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib @ARGPINCS@
scan_LDADD = ./libmh.a @ARGPLIBS@ ../mailbox/libmailbox.la ../lib/libmailutils.a -lcurses -ltermcap
YFLAGS=-vt

