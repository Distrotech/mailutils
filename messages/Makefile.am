AUTOMAKE_OPTIONS = ../lib/ansi2knr
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib @ARGPINCS@

bin_PROGRAMS = messages

messages_DEPENDENCIES = ../mailbox/libmailbox.la
messages_LDADD = @ARGPLIBS@ ../mailbox/libmailbox.la ../lib/libmailutils.a
