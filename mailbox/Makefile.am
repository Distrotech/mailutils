# Use automake to process this file -*-Makefile-*-

AUTOMAKE_OPTIONS = ../lib/ansi2knr

INCLUDES = -I${srcdir}/include/public \
         -I${srcdir}/include/private
CFLAGS = -Wall -pedantic -g -DTESTING

SUBDIRS = include

lib_LTLIBRARIES = libmailbox.la

libmailbox_la_SOURCES = \
_cpystr.c attachment.c attribute.c auth.c body.c file_stream.c \
header.c io.c locker.c mailbox.c mailer.c mbx_default.c mbx_imap.c mbx_mbox.c \
mbx_mdir.c mbx_mh.c mbx_mmdf.c mbx_pop.c mbx_unix.c \
message.c mime.c registrar.c tcp.c trans_stream.c \
url.c url_file.c url_imap.c url_mail.c url_mbox.c url_mdir.c \
url_mh.c url_mmdf.c url_pop.c url_unix.c

libmailbox_la_LDFLAGS = -version-info 0:0:0

