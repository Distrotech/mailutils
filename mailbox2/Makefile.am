# Use automake to process this file -*-Makefile-*-

AUTOMAKE_OPTIONS = ../lib/ansi2knr

#INCLUDES = -I${top_srcdir}/include
INCLUDES = -I${top_srcdir}/mailbox2/include

SUBDIRS = include pop3 mbox


lib_LTLIBRARIES = libmailbox.la

libmailbox_la_SOURCES = \
 address.c    \
 attribute.c  \
 authority.c  \
 bstream.c    \
 dotlock.c    \
 envelope.c   \
 folder.c     \
 fstream.c    \
 header.c     \
 iterator.c   \
 list.c       \
 locker.c     \
 mailbox.c    \
 md5-rsa.c    \
 memstream.c  \
 message.c    \
 mstream.c    \
 mutil.c      \
 observable.c \
 observer.c   \
 parse822.c   \
 pticket.c    \
 stream.c     \
 tcpstream.c  \
 ticket.c
