CFLAGS = -Wall -pedantic -g -DTESTING
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib

bin_PROGRAMS = mail

mail_DEPENDENCIES = ../mailbox/libmailbox.la
mail_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.a
