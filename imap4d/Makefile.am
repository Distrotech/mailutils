AUTOMAKE_OPTIONS = ../lib/ansi2knr
INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/include

sbin_PROGRAMS =	imap4d

imap4d_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.a @AUTHLIBS@
imap4d_SOURCES = imap4d.c imap4d.h capability.c noop.c logout.c authenticate.c \
	login.c select.c examine.c create.c delete.c rename.c subscribe.c \
	unsubscribe.c list.c lsub.c status.c append.c check.c close.c \
	expunge.c search.c fetch.c store.c sync.c copy.c uid.c util.c commands.c
