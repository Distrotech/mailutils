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
 dattribute.c \
 debug.c      \
 dotlock.c    \
 envelope.c   \
 fdstream.c   \
 folder.c     \
 fstream.c    \
 header.c     \
 iterator.c   \
 list.c       \
 lockfile.c   \
 mailbox.c    \
 md5-rsa.c    \
 memstream.c  \
 message.c    \
 mapstream.c  \
 mutil.c      \
 observable.c \
 observer.c   \
 parse822.c   \
 pticket.c    \
 refcount.c   \
 sdebug.c     \
 stream.c     \
 tcpstream.c  \
 ticket.c
