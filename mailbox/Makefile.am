# This file is part of GNU Mailutils
# Copyright (C) 2000,2001,2002 Free Software Foundation
# See file COPYING in the distribution root directory for copying conditions.

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/mailbox -I${top_srcdir}/mailbox/include -I${top_srcdir}/include/mailutils/gnu
AM_CFLAGS = -DSITE_VIRTUAL_PWDDIR=\"@SITE_VIRTUAL_PWDDIR@\"

SUBDIRS = include testsuite

lib_LTLIBRARIES = libmailbox.la
bin_PROGRAMS = mailutils-config

EXTRA_DIST = mbx_mboxscan.c strtok_r.c \
 strndup.c strnlen.c strchrnul.c argp-ba.c argp-eexst.c \
 argp-fmtstream.c argp-fs-xinl.c argp-help.c argp-parse.c argp-pv.c \
 argp-pvh.c argp-xinl.c getline.c getline.h

noinst_HEADERS=argp-fmtstream.h argp-namefrob.h md5.h

libmailbox_la_SOURCES = \
address.c \
argcv.c \
attachment.c \
attribute.c \
auth.c \
body.c \
date.c \
debug.c \
envelope.c \
file_stream.c \
filter.c \
filter_rfc822.c \
filter_trans.c \
folder.c \
folder_imap.c \
folder_mbox.c \
folder_mh.c \
folder_pop.c \
header.c \
iterator.c \
list.c \
locker.c \
mailbox.c \
mailer.c \
mapfile_stream.c \
mbx_default.c \
mbx_file.c \
mbx_imap.c \
mbx_mbox.c \
mbx_mh.c \
mbx_pop.c \
md5.c \
message.c \
memory_stream.c \
mime.c \
monitor.c \
mu_argp.c \
mu_auth.c \
mutil.c \
muerror.c \
muerrno.c \
observer.c \
parse822.c \
property.c \
registrar.c \
sendmail.c \
smtp.c \
stream.c \
system.c \
tcp.c \
ticket.c \
url.c \
url_file.c \
url_imap.c \
url_mbox.c \
url_mh.c \
url_path.c \
url_pop.c \
url_sendmail.c \
url_smtp.c \
wicket.c

libmailbox_la_DEPENDENCIES = @MU_LTLIBOBJS@
libmailbox_la_LIBADD = @MU_LTLIBOBJS@
libmailbox_la_LDFLAGS = -version-info 0:0:0

mailutils_config_SOURCES = mailutils-config.c
mailutils_config_LDADD = ./libmailbox.la
mailutils_config_CFLAGS = -DCOMPILE_FLAGS="\"-I$(includedir)\"" -DLINK_FLAGS="\"-L$(libdir)\"" -DAUTHLIBS="\"$(MYSQLLIBS)\"" -DGUILE_LIBS="\"$(GUILE_LIBS)\""
