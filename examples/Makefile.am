AUTOMAKE_OPTIONS = ../lib/ansi2knr
noinst_PROGRAMS = addr url-parse sfrom mimetest muemail msg-send

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/lib
AM_LDFLAGS = ../mailbox/libmailbox.la ../lib/libmailutils.la 

EXTRA_DIST = comsat.conf dot.biffrc gnu-imap4d.pam gnu-pop3d.pam\
 mailutils.rc reply.scm