AUTOMAKE_OPTIONS = ../lib/ansi2knr
INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/include

sbin_PROGRAMS = comsatd

comsatd_SOURCES = action.c cfg.c comsat.c comsat.h

comsatd_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.a
