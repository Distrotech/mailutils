# Use automake to process this file -*-Makefile-*-

AUTOMAKE_OPTIONS = ../lib/ansi2knr

#INCLUDES = -I${top_srcdir}/include
INCLUDES = -I${top_srcdir}/mailbox2/include

EXTRA_DIST = valid.h

lib_LTLIBRARIES = libmbox.la

libmbox_la_SOURCES = \
 mbox_attribute.c    \
 mbox_blines.c       \
 mbox_bsize.c        \
 mbox_bstream.c      \
 mbox_carrier.c      \
 mbox_cb.c           \
 mbox_changed.c      \
 mbox_close.c        \
 mbox_create.c       \
 mbox_destroy.c      \
 mbox_expunge.c      \
 mbox_hcache.c       \
 mbox_hlines.c       \
 mbox_hsize.c        \
 mbox_hstream.c      \
 mbox_open.c         \
 mbox_scan.c         \
 mbox_separator.c    \
 mbox_size.c         \
 mbox_stream.c       \
 mbox_uid.c          \
 mbox_uidnext.c      \
 mbox_uidvalidity.c
