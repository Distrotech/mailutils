# Use automake to process this file -*-Makefile-*-

AUTOMAKE_OPTIONS = ../lib/ansi2knr

#INCLUDES = -I${top_srcdir}/include
INCLUDES = -I${top_srcdir}/mailbox2/include

SUBDIRS = include pop3 mbox


lib_LTLIBRARIES = libmailbox.la

libmailbox_la_SOURCES = \
 bstream.c  \
 fstream.c  \
 iterator.c \
 md5-rsa.c  \
 mstream.c  \
 stream.c   \
 tcp.c
