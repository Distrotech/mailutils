# Use automake to process this file -*-Makefile-*-

AUTOMAKE_OPTIONS = ../lib/ansi2knr

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/mailbox/include -I${top_srcdir}/lib
CFLAGS = -Wall -pedantic -g -DTESTING

SUBDIRS = include

lib_LTLIBRARIES = libmailbox.la

EXTRA_DIST = mbx_mboxscan.c

libmailbox_la_SOURCES = \
attachment.c \
attribute.c \
auth.c \
bio.c  \
body.c \
debug.c \
file_stream.c \
header.c \
iterator.c \
list.c \
locker.c \
mailbox.c \
mailer.c \
mapfile_stream.c \
mbx_default.c \
mbx_file.c \
mbx_mbox.c \
mbx_pop.c \
message.c \
mime.c \
misc.c
observer.c \
registrar.c \
sendmail.c \
smtp.c \
stream.c \
tcp.c \
trans_stream.c \
transcode.c \
url.c \
url_file.c \
url_mbox.c \
url_path.c \
url_pop.c \
url_sendmail.c \
url_smtp.c


libmailbox_la_LDFLAGS = -version-info 0:0:0

