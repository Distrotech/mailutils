## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2000, 2001, 2002, 2004, 2005, 2006 
##   Free Software Foundation, Inc.
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
##   Foundation, Inc. 51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

INCLUDES = @MU_COMMON_INCLUDES@ -I${top_srcdir}/libproto/include @INTLINCS@

AM_CPPFLAGS = -DSITE_VIRTUAL_PWDDIR=\"@SITE_VIRTUAL_PWDDIR@\" -DGNULIB_ARGP_EXTERN_BASENAME=1
YLWRAP = $(SHELL) $(top_srcdir)/scripts/ylwrap
AM_YFLAGS=-vt

SUBDIRS = testsuite 

lib_LTLIBRARIES = libmailutils.la

localedir = $(datadir)/locale
DEFS = @DEFS@ -DLOCALEDIR=\"$(localedir)\"

EXTRA_DIST = @MU_EXTRA_DIST@ errors muerrno.cin parsedate.y fgetpwent.c

libmailutils_la_SOURCES = \
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
message_stream.c \
mime.c \
monitor.c \
mu_argp.c \
mu_auth.c \
mu_umaxtostr.h \
mu_umaxtostr.c \
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

libmailutils_la_DEPENDENCIES = @MU_LTLIBOBJS@
libmailutils_la_LIBADD = @MU_LTLIBOBJS@ @MU_COMMON_LIBRARIES@
libmailutils_la_LDFLAGS = -version-info @VI_CURRENT@:@VI_REVISION@:@VI_AGE@

##:## EOF marker for gnulib-sync script. Please, do not remove ##:##
## Do not change anything below this line ##

BUILT_SOURCES += $(ALLOCA_H)
EXTRA_DIST += alloca_.h

# We need the following in order to create <alloca.h> when the system
# doesn't have one that works with the given compiler.
alloca.h: alloca_.h
	cp $(srcdir)/alloca_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += alloca.h alloca.h-t

libmailutils_la_SOURCES += argp.h argp-ba.c argp-eexst.c \
           argp-fmtstream.c argp-fmtstream.h argp-fs-xinl.c argp-help.c \
           argp-namefrob.h argp-parse.c argp-pin.c argp-pv.c argp-pvh.c \
           argp-xinl.c



libmailutils_la_SOURCES += strtok_r.h


BUILT_SOURCES += $(GLOB_H)

# We need the following in order to create <glob.h> when the system
# doesn't have one that works with the given compiler.
glob.h: glob_.h
	cp $(srcdir)/glob_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += glob.h glob.h-t

EXTRA_DIST += inttostr.c


BUILT_SOURCES += $(FNMATCH_H)
EXTRA_DIST += fnmatch_.h fnmatch_loop.c

# We need the following in order to create <fnmatch.h> when the system
# doesn't have one that supports the required API.
fnmatch.h: fnmatch_.h
	cp $(srcdir)/fnmatch_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += fnmatch.h fnmatch.h-t



BUILT_SOURCES += $(GETOPT_H)
EXTRA_DIST += getopt_.h getopt_int.h

# We need the following in order to create <getopt.h> when the system
# doesn't have one that works with the given compiler.
getopt.h: getopt_.h
	cp $(srcdir)/getopt_.h $@-t
	mv $@-t $@
MOSTLYCLEANFILES += getopt.h getopt.h-t

libmailutils_la_SOURCES += gettext.h


libmailutils_la_SOURCES += mbchar.h

libmailutils_la_SOURCES += mbuiter.h



libmailutils_la_SOURCES += minmax.h


libmailutils_la_SOURCES += size_max.h


BUILT_SOURCES += $(STDBOOL_H)
EXTRA_DIST += stdbool_.h

# We need the following in order to create <stdbool.h> when the system
# doesn't have one that works.
stdbool.h: stdbool_.h
	sed -e 's/@''HAVE__BOOL''@/$(HAVE__BOOL)/g' < $(srcdir)/stdbool_.h > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += stdbool.h stdbool.h-t

BUILT_SOURCES += $(STDINT_H)
EXTRA_DIST += stdint_.h

# We need the following in order to create <stdint.h> when the system
# doesn't have one that works with the given compiler.
stdint.h: stdint_.h
	sed -e 's/@''HAVE_LONG_64BIT''@/$(HAVE_LONG_64BIT)/g;s/@''HAVE_LONG_LONG_64BIT@/$(HAVE_LONG_LONG_64BIT)/g' < $(srcdir)/stdint_.h > $@-t
	mv $@-t $@
MOSTLYCLEANFILES += stdint.h stdint.h-t

libmailutils_la_SOURCES += strcase.h

libmailutils_la_SOURCES += strchrnul.h




libmailutils_la_SOURCES += strnlen1.h strnlen1.c

BUILT_SOURCES += $(SYSEXITS_H)
EXTRA_DIST += sysexit_.h

# We need the following in order to create <sysexits.h> when the system
# doesn't have one that works with the given compiler.
sysexits.h: sysexit_.h
	cp $(srcdir)/sysexit_.h sysexits.h-t
	mv sysexits.h-t sysexits.h
MOSTLYCLEANFILES += sysexits.h sysexits.h-t

libmailutils_la_SOURCES += printf-args.h printf-parse.h vasnprintf.h

libmailutils_la_SOURCES += vsnprintf.h

libmailutils_la_SOURCES += xsize.h

EXTRA_DIST += unlocked-io.h getndelim2.c fnmatch.c getdelim.c getdelim.h getdelim.c getopt.c getopt1.c mbchar.c mempcpy.c mempcpy.h mempcpy.c regex.c strcasecmp.c strncasecmp.c getlogin_r.c getlogin_r.c inttostr.c inttostr.h intprops.h imaxtostr.c offtostr.c umaxtostr.c imaxtostr.c offtostr.c umaxtostr.c strchrnul.c strdup.c strdup.h strdup.c strndup.c strndup.h strndup.c strnlen.c strnlen.h strnlen.c strtok_r.c vasnprintf.c printf-args.c printf-parse.c asnprintf.c vsnprintf.c getline.c getline.c getline.h getline.c glob.c glob_.h glob-libc.h glob.c md5.c md5.h md5.c memchr.c stat-macros.h
