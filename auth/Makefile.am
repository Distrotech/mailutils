# Use automake to process this file -*-Makefile-*-

AUTOMAKE_OPTIONS = ../lib/ansi2knr

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/mailbox/include -I${top_srcdir}/lib
AM_CFLAGS = -DSITE_VIRTUAL_PWDDIR=\"@SITE_VIRTUAL_PWDDIR@\"

lib_LTLIBRARIES = libmuauth.la

libmuauth_la_SOURCES = \
pam.c \
system.c \
sql.c \
virtual.c 

libmuauth_la_LDFLAGS = -version-info 0:0:0
