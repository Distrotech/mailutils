CFLAGS = -Wall -pedantic -g -DTESTING
INCLUDES = -I$(top_srcdir)/libmailbox -I$(top_srcdir)/lib

bin_PROGRAMS = mail

mail_DEPENDENCIES = ../libmailbox/libmailbox.la
mail_LDADD = ../libmailbox/libmailbox.la ../lib/libmailutils.a
