AUTOMAKE_OPTIONS = ../lib/ansi2knr
INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/include -I$(top_srcdir)/libmu_scm 

libexec_PROGRAMS = mail.remote
mail_remote_SOURCES = mail.remote.c mail.remote.h

mail_remote_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.la

