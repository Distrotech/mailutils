## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2000, 2001, 2002 Free Software Foundation, Inc.
##
##   GNU Mailtuils is free software; you can redistribute it and/or
##   modify it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 2, or (at
##   your option) any later version.
##
##   This program is distributed in the hope that it will be useful, but
##   WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##   General Public License for more details.
##
##   You should have received a copy of the GNU General Public License
##   along with this program; if not, write to the Free Software
##   Foundation, Inc.
##   59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/mailbox -I${top_srcdir}/mailbox/include -I${top_srcdir}/include/mailutils/gnu @INTLINCS@
AM_CFLAGS = -DSITE_VIRTUAL_PWDDIR=\"@SITE_VIRTUAL_PWDDIR@\"

SUBDIRS = include testsuite

lib_LTLIBRARIES = libmailbox.la
bin_PROGRAMS = mailutils-config

localedir = $(datadir)/locale
DEFS = @DEFS@ -DLOCALEDIR=\"$(localedir)\"

EXTRA_DIST = mbx_mboxscan.c strtok_r.c \
 strndup.c strnlen.c strchrnul.c argp-ba.c argp-eexst.c \
 argp-fmtstream.c argp-fs-xinl.c argp-help.c argp-parse.c argp-pv.c \
 argp-pvh.c argp-xinl.c getline.c getline.h pin.c

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
mbx_maildir.c \
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
nls.c \
observer.c \
parse822.c \
property.c \
registrar.c \
rfc2047.c \
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
libmailbox_la_LIBADD = @MU_LTLIBOBJS@ @LTLIBINTL@
libmailbox_la_LDFLAGS = -version-info 0:0:0

mailutils_config_SOURCES = mailutils-config.c
mailutils_config_LDADD = ./libmailbox.la ../lib/libmailutils.la @LTLIBINTL@
mailutils_config_CFLAGS = -DCOMPILE_FLAGS="\"-I$(includedir)\"" \
  -I$(top_srcdir)/intl \
  -I${top_srcdir}/lib \
  -DLINK_FLAGS="\"-L$(libdir)\"" \
  -DAUTHLIBS="\"`echo $(AUTHLIBS) | sed 's/.*libmuauth.la *//'`\"" \
  -DGUILE_LIBS="\"$(GUILE_LIBS)\"" \
  -DI18NLIBS="\"`test $(BUILD_INCLUDED_LIBINTL) = no && echo $(LIBINTL)`\""

