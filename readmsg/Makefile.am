AUTOMAKE_OPTIONS = ../lib/ansi2knr
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib
SUBDIRS = testsuite

bin_PROGRAMS = readmsg

readmsg_SOURCES = readmsg.c msglist.c readmsg.h
readmsg_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.la
