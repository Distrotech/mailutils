# Use automake to process this file -*-Makefile-*-

CPPFLAGS = -Wall

if INSTALL_MAILBOX
include_HEADERS = mailbox.h
lib_LTLIBRARIES = libmailbox.la
else
noinst_HEADERS = mailbox.h
noinst_LTLIBRARIES = libmailbox.la
endif

libmailbox_la_SOURCES = mailbox.c unixmbox.c unixmbox.h
libmailbox_la_LDFLAGS = -version-info 0:0:0

