## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2000, 2001, 2002, 2004, 2005 Free Software Foundation, Inc.
##
##   GNU Mailutils is free software; you can redistribute it and/or
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
##   Foundation, Inc. 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/mailbox \
 -I${top_srcdir}/mailbox/include -I${top_builddir}/include/mailutils/gnu \
 @INTLINCS@

AM_CFLAGS = -DSITE_VIRTUAL_PWDDIR=\"@SITE_VIRTUAL_PWDDIR@\"
YLWRAP = $(SHELL) $(top_srcdir)/scripts/ylwrap
AM_YFLAGS=-vt

SUBDIRS = include mbox pop nntp imap mh maildir testsuite 

lib_LTLIBRARIES = libmailbox.la
bin_PROGRAMS = mailutils-config

localedir = $(datadir)/locale
DEFS = @DEFS@ -DLOCALEDIR=\"$(localedir)\"

EXTRA_DIST = errors muerrno.cin parsedate.y pin.c fgetpwent.c

libmailbox_la_SOURCES = \
address.c \
amd.c \
argcv.c \
attachment.c \
attribute.c \
auth.c \
body.c \
daemon.c \
date.c \
debug.c \
envelope.c \
file_stream.c \
filter.c \
filter_iconv.c \
filter_rfc822.c \
filter_trans.c \
folder.c \
header.c \
iterator.c \
list.c \
locale.c \
locker.c \
mailbox.c \
mailcap.c \
mailer.c \
mapfile_stream.c \
mbx_default.c \
message.c \
memory_stream.c \
mime.c \
monitor.c \
mu_argp.c \
mu_auth.c \
munre.c \
mutil.c \
muerror.c \
muerrno.c \
nls.c \
observer.c \
parse822.c \
parsedate.c \
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
url_sendmail.c \
url_smtp.c \
wicket.c

BUILT_SOURCES = parsedate.c muerrno.c
MOSTLYCLEANFILES=

parsedate.c: $(srcdir)/parsedate.y
	$(YLWRAP) "$(YACC) $(AM_YFLAGS)" $< \
	          y.tab.c parsedate.c y.output parsedate.y.output \
                  -- -yy pd_yy

muerrno.c: errors muerrno.cin
	$(AWK) -f $(top_srcdir)/scripts/generr.awk $^ > $@

libmailbox_la_DEPENDENCIES = @MU_LTLIBOBJS@
libmailbox_la_LIBADD = @MU_LTLIBOBJS@ @MU_COMMON_LIBRARIES@
libmailbox_la_LDFLAGS = -version-info 0:0:0

mailutils_config_SOURCES = mailutils-config.c
mailutils_config_LDADD = ./libmailbox.la ../lib/libmailutils.la @MU_COMMON_LIBRARIES@
mailutils_config_CFLAGS = -DCOMPILE_FLAGS="\"-I$(includedir)\"" \
  -I$(top_srcdir)/intl \
  -I${top_srcdir}/lib \
  -DLINK_FLAGS="\"-L$(libdir)\"" \
  -DLINK_POSTFLAGS="\"$(LTLIBICONV)\"" \
  -DAUTHLIBS="\"`echo $(AUTHLIBS) | sed 's/.*libmuauth.la *//'`\"" \
  -DGUILE_LIBS="\"$(GUILE_LIBS)\"" \
  -DI18NLIBS="\"`test $(BUILD_INCLUDED_LIBINTL) = no && echo $(LIBINTL)`\""

##:## EOF marker for gnulib-sync script. Please, do not remove ##:##
## Do not change anything below this line ##

BUILT_SOURCES += $(ALLOCA_H)
EXTRA_DIST += alloca_.h

# We need the following in order to create an <alloca.h> when the system
# doesn't have one that works with the given compiler.
all-local $(libmailbox_la_OBJECTS): $(ALLOCA_H)
alloca.h: alloca_.h
	cp $(srcdir)/alloca_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += alloca.h alloca.h-t

libmailbox_la_SOURCES += argp.h argp-ba.c argp-eexst.c \
           argp-fmtstream.c argp-fmtstream.h argp-fs-xinl.c argp-help.c \
           argp-namefrob.h argp-parse.c argp-pv.c argp-pvh.c argp-xinl.c

libmailbox_la_SOURCES += getline.h
EXTRA_DIST += getndelim2.h getndelim2.c

libmailbox_la_SOURCES += regex.h

libmailbox_la_SOURCES += strtok_r.h

libmailbox_la_SOURCES += md5.h md5.c


BUILT_SOURCES += $(GETOPT_H)
EXTRA_DIST += getopt_.h getopt_int.h

# We need the following in order to create an <getopt.h> when the system
# doesn't have one that works with the given compiler.
all-local $(libmailbox_la_OBJECTS): $(GETOPT_H)
getopt.h: getopt_.h
	cp $(srcdir)/getopt_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += getopt.h getopt.h-t

libmailbox_la_SOURCES += gettext.h


libmailbox_la_SOURCES += minmax.h


libmailbox_la_SOURCES += strcase.h

libmailbox_la_SOURCES += strchrnul.h

libmailbox_la_SOURCES += strndup.h


EXTRA_DIST += sysexit_.h

# We need the following in order to create a <sysexits.h> when the system
# doesn't have one that works with the given compiler.
all-local $(libmailbox_la_OBJECTS): $(SYSEXITS_H)
sysexits.h: sysexit_.h
	cp $(srcdir)/sysexit_.h sysexits.h-t
	mv sysexits.h-t sysexits.h
MOSTLYCLEANFILES += sysexits.h sysexits.h-t

libmailbox_la_SOURCES += unlocked-io.h

libmailbox_la_SOURCES += printf-args.h printf-parse.h vasnprintf.h

libmailbox_la_SOURCES += vsnprintf.h

libmailbox_la_SOURCES += xsize.h

EXTRA_DIST += regex.c getline.c getline.c getndelim2.c getndelim2.c getopt.c getopt1.c mempcpy.c mempcpy.h mempcpy.c strcasecmp.c strncasecmp.c strchrnul.c strndup.c strnlen.c strtok_r.c vasnprintf.c printf-args.c printf-parse.c asnprintf.c vsnprintf.c
