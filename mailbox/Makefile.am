# Use automake to process this file -*-Makefile-*-

AUTOMAKE_OPTIONS = ../lib/ansi2knr

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/mailbox/include -I${srcdir}/lib
CFLAGS = -Wall -pedantic -g -DTESTING

SUBDIRS = include

lib_LTLIBRARIES = libmailbox.la

EXTRA_DIST = mbx_unixscan.c

libmailbox_la_SOURCES = \
_cpystr.c attachment.c attribute.c auth.c body.c file_stream.c \
header.c stream.c locker.c mailbox.c mailer.c mapfile_stream.c \
mbx_default.c mbx_imap.c mbx_mbox.c mbx_mdir.c mbx_mh.c mbx_mmdf.c \
mbx_pop.c mbx_unix.c message.c mime.c registrar.c tcp.c trans_stream.c \
url.c url_file.c url_imap.c url_mail.c url_mbox.c url_mdir.c \
url_mh.c url_mmdf.c url_pop.c url_unix.c

libmailbox_la_LDFLAGS = -version-info 0:0:0

