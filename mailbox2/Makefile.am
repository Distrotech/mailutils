# Use automake to process this file -*-Makefile-*-

AUTOMAKE_OPTIONS = ../lib/ansi2knr

#INCLUDES = -I${top_srcdir}/include
INCLUDES = -I${top_srcdir}/mailbox2/include

SUBDIRS = include pop3 mbox


lib_LTLIBRARIES = libmailbox.la

libmailbox_la_SOURCES = \
 address.c    \
 attribute.c  \
 bstream.c    \
 envelope.c   \
 fstream.c    \
 iterator.c   \
 list.c       \
 md5-rsa.c    \
 mstream.c    \
 observable.c \
 observer.c   \
 parse822.c   \
 stream.c     \
 tcpstream.c
