AUTOMAKE_OPTIONS = ../lib/ansi2knr
INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/include @ARGPINCS@

sbin_PROGRAMS = comsatd

comsatd_SOURCES = action.c cfg.c comsat.c comsat.h

comsatd_LDADD = @ARGPLIBS@ ../mailbox/libmailbox.la ../lib/libmailutils.a
