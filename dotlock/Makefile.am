AUTOMAKE_OPTIONS = ../lib/ansi2knr
INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/include -I$(top_srcdir)/libmu_scm @ARGPINCS@

bin_PROGRAMS = dotlock
#dotlock_SOURCES = dotlock.c

dotlock_LDADD = ../mailbox/libmailbox.la @ARGPLIBS@ ../lib/libmailutils.la

install-exec-hook:
	for i in $(bin_PROGRAMS); do\
		chown root:mail $(DESTDIR)$(bindir)/$$i;\
		chmod 2755 $(DESTDIR)$(bindir)/$$i;\
	done
