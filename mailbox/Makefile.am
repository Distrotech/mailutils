# Use automake to process this file -*-Makefile-*-

AUTOMAKE_OPTIONS = ../lib/ansi2knr

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/mailbox/include -I${top_srcdir}/lib
AM_CFLAGS = -DSITE_VIRTUAL_PWDDIR=\"@SITE_VIRTUAL_PWDDIR@\"

SUBDIRS = include testsuite

lib_LTLIBRARIES = libmailbox.la

EXTRA_DIST = mbx_mboxscan.c md5-rsa.h

libmailbox_la_SOURCES = \
address.c \
attachment.c \
attribute.c \
auth.c \
body.c \
debug.c \
envelope.c \
file_stream.c \
filter.c \
filter_rfc822.c \
filter_trans.c \
folder.c \
folder_imap.c \
folder_mbox.c \
folder_mh.c \
folder_pop.c \
header.c \
iterator.c \
list.c \
locker.c \
mailbox.c \
mailer.c \
mapfile_stream.c \
mbx_default.c \
mbx_file.c \
mbx_imap.c \
mbx_mbox.c \
mbx_mh.c \
mbx_pop.c \
md5-rsa.c \
message.c \
memory_stream.c \
mime.c \
monitor.c \
mu_argp.c \
mu_auth.c \
mutil.c \
muerror.c \
muerrno.c \
observer.c \
parse822.c \
property.c \
registrar.c \
sendmail.c \
smtp.c \
stream.c \
system.c \
tcp.c \
ticket.c \
url.c \
url_file.c \
url_imap.c \
url_mbox.c \
url_mh.c \
url_path.c \
url_pop.c \
url_sendmail.c \
url_smtp.c \
wicket.c

libmailbox_la_LDFLAGS = -version-info 0:0:0
