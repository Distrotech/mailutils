# Use automake to process this file -*-Makefile-*-

CFLAGS = -Wall -pedantic -g -DTESTING

include_HEADERS = mailbox.h
lib_LTLIBRARIES = libmailbox.la

libmailbox_la_SOURCES = mailbox.c unixmbox.c unixmbox.h
libmailbox_la_LDFLAGS = -version-info 0:0:0

