AUTOMAKE_OPTIONS = ../lib/ansi2knr
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib

bin_PROGRAMS = readmsg

readmsg_SOURCES = readmsg.c msglist.c readmsg.h
readmsg_DEPENDENCIES = ../mailbox/libmailbox.la
readmsg_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.a
