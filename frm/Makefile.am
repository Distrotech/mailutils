AUTOMAKE_OPTIONS = ../lib/ansi2knr
CFLAGS = -Wall -pedantic -g
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib

bin_PROGRAMS = frm

frm_DEPENDENCIES = ../mailbox/libmailbox.la
frm_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.a
