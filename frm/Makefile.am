CFLAGS = -Wall -pedantic -g -DTESTING
INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib

bin_PROGRAMS = frm

from_DEPENDENCIES = ../mailbox/libmailbox.la
from_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.a
