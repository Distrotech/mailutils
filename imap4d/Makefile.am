INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/libmailbox
CPPFLAGS = -Wall -pedantic -ansi

sbin_PROGRAMS =		imap4d

imap4d_SOURCES = 	capability.c \
			login.c \
			logout.c \
			main.c \
			noop.c

imap4d_LDADD =		../libmailbox/libmailbox.la \
			../lib/libmailutils.a \
			@AUTHLIBS@

noinst_HEADERS = 	capability.h \
			imap.h \
			imap_commands.h \
			imap_types.h \
			login.h \
			logout.h \
			noop.h

