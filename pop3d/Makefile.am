INCLUDES =-I$(srcdir) -I$(top_srcdir)/lib -I$(top_srcdir)/libmailbox

sbin_PROGRAMS = pop3d

pop3d_DEPENDENDENCIES = ../libmailbox/libmailbox.la ../lib/libgetopt.la\
	../libsrc/libmd5.la
pop3d_SOURCES = apop.c auth.c capa.c dele.c extra.c pop3d.c pop3d.h\
	list.c noop.c quit.c retr.c rset.c stat.c top.c uidl.c user.c signal.c

pop3d_LDADD = ../libmailbox/libmailbox.la ../lib/libmailutils.a @AUTHLIBS@
