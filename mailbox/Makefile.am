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

AM_CPPFLAGS = -DSITE_VIRTUAL_PWDDIR=\"@SITE_VIRTUAL_PWDDIR@\" 
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

EXTRA_DIST += intprops.h


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
	sed -e 's/@''HAVE_WCHAR_H''@/$(HAVE_WCHAR_H)/g' \
	    -e 's/@''HAVE_STDINT_H''@/$(HAVE_STDINT_H)/g' \
	    -e 's|@''FULL_PATH_STDINT_H''@|$(FULL_PATH_STDINT_H)|g' \
	    -e 's/@''HAVE_INTTYPES_H''@/$(HAVE_INTTYPES_H)/g' \
	    -e 's|@''FULL_PATH_INTTYPES_H''@|$(FULL_PATH_INTTYPES_H)|g' \
	    -e 's/@''HAVE_SYS_INTTYPES_H''@/$(HAVE_SYS_INTTYPES_H)/g' \
	    -e 's/@''HAVE_SYS_BITYPES_H''@/$(HAVE_SYS_BITYPES_H)/g' \
	    -e 's/@''HAVE_LONG_64BIT''@/$(HAVE_LONG_64BIT)/g' \
	    -e 's/@''HAVE_LONG_LONG_64BIT''@/$(HAVE_LONG_LONG_64BIT)/g' \
	    -e 's/@''HAVE_INT8_T''@/$(HAVE_INT8_T)/g' \
	    -e 's/@''HAVE_INT16_T''@/$(HAVE_INT16_T)/g' \
	    -e 's/@''HAVE_INT32_T''@/$(HAVE_INT32_T)/g' \
	    -e 's/@''HAVE_INT64_T''@/$(HAVE_INT64_T)/g' \
	    -e 's/@''HAVE_UINT8_T''@/$(HAVE_UINT8_T)/g' \
	    -e 's/@''HAVE_UINT16_T''@/$(HAVE_UINT16_T)/g' \
	    -e 's/@''HAVE_UINT32_T''@/$(HAVE_UINT32_T)/g' \
	    -e 's/@''HAVE_UINT64_T''@/$(HAVE_UINT64_T)/g' \
	    -e 's/@''HAVE_INT_LEAST8_T''@/$(HAVE_INT_LEAST8_T)/g' \
	    -e 's/@''HAVE_INT_LEAST16_T''@/$(HAVE_INT_LEAST16_T)/g' \
	    -e 's/@''HAVE_INT_LEAST32_T''@/$(HAVE_INT_LEAST32_T)/g' \
	    -e 's/@''HAVE_INT_LEAST64_T''@/$(HAVE_INT_LEAST64_T)/g' \
	    -e 's/@''HAVE_UINT_LEAST8_T''@/$(HAVE_UINT_LEAST8_T)/g' \
	    -e 's/@''HAVE_UINT_LEAST16_T''@/$(HAVE_UINT_LEAST16_T)/g' \
	    -e 's/@''HAVE_UINT_LEAST32_T''@/$(HAVE_UINT_LEAST32_T)/g' \
	    -e 's/@''HAVE_UINT_LEAST64_T''@/$(HAVE_UINT_LEAST64_T)/g' \
	    -e 's/@''HAVE_INT_FAST8_T''@/$(HAVE_INT_FAST8_T)/g' \
	    -e 's/@''HAVE_INT_FAST16_T''@/$(HAVE_INT_FAST16_T)/g' \
	    -e 's/@''HAVE_INT_FAST32_T''@/$(HAVE_INT_FAST32_T)/g' \
	    -e 's/@''HAVE_INT_FAST64_T''@/$(HAVE_INT_FAST64_T)/g' \
	    -e 's/@''HAVE_UINT_FAST8_T''@/$(HAVE_UINT_FAST8_T)/g' \
	    -e 's/@''HAVE_UINT_FAST16_T''@/$(HAVE_UINT_FAST16_T)/g' \
	    -e 's/@''HAVE_UINT_FAST32_T''@/$(HAVE_UINT_FAST32_T)/g' \
	    -e 's/@''HAVE_UINT_FAST64_T''@/$(HAVE_UINT_FAST64_T)/g' \
	    -e 's/@''HAVE_INTPTR_T''@/$(HAVE_INTPTR_T)/g' \
	    -e 's/@''HAVE_UINTPTR_T''@/$(HAVE_UINTPTR_T)/g' \
	    -e 's/@''HAVE_INTMAX_T''@/$(HAVE_INTMAX_T)/g' \
	    -e 's/@''HAVE_UINTMAX_T''@/$(HAVE_UINTMAX_T)/g' \
	    -e 's/@''BITSIZEOF_UNSIGNED_INT''@/$(BITSIZEOF_UNSIGNED_INT)/g' \
	    -e 's/@''BITSIZEOF_LONG''@/$(BITSIZEOF_LONG)/g' \
	    -e 's/@''BITSIZEOF_UNSIGNED_LONG''@/$(BITSIZEOF_UNSIGNED_LONG)/g' \
	    -e 's/@''BITSIZEOF_INT8_T''@/$(BITSIZEOF_INT8_T)/g' \
	    -e 's/@''BITSIZEOF_INT16_T''@/$(BITSIZEOF_INT16_T)/g' \
	    -e 's/@''BITSIZEOF_INT32_T''@/$(BITSIZEOF_INT32_T)/g' \
	    -e 's/@''BITSIZEOF_INT64_T''@/$(BITSIZEOF_INT64_T)/g' \
	    -e 's/@''BITSIZEOF_UINT8_T''@/$(BITSIZEOF_UINT8_T)/g' \
	    -e 's/@''BITSIZEOF_UINT16_T''@/$(BITSIZEOF_UINT16_T)/g' \
	    -e 's/@''BITSIZEOF_UINT32_T''@/$(BITSIZEOF_UINT32_T)/g' \
	    -e 's/@''BITSIZEOF_UINT64_T''@/$(BITSIZEOF_UINT64_T)/g' \
	    -e 's/@''BITSIZEOF_INT_LEAST8_T''@/$(BITSIZEOF_INT_LEAST8_T)/g' \
	    -e 's/@''BITSIZEOF_INT_LEAST16_T''@/$(BITSIZEOF_INT_LEAST16_T)/g' \
	    -e 's/@''BITSIZEOF_INT_LEAST32_T''@/$(BITSIZEOF_INT_LEAST32_T)/g' \
	    -e 's/@''BITSIZEOF_INT_LEAST64_T''@/$(BITSIZEOF_INT_LEAST64_T)/g' \
	    -e 's/@''BITSIZEOF_UINT_LEAST8_T''@/$(BITSIZEOF_UINT_LEAST8_T)/g' \
	    -e 's/@''BITSIZEOF_UINT_LEAST16_T''@/$(BITSIZEOF_UINT_LEAST16_T)/g' \
	    -e 's/@''BITSIZEOF_UINT_LEAST32_T''@/$(BITSIZEOF_UINT_LEAST32_T)/g' \
	    -e 's/@''BITSIZEOF_UINT_LEAST64_T''@/$(BITSIZEOF_UINT_LEAST64_T)/g' \
	    -e 's/@''BITSIZEOF_INT_FAST8_T''@/$(BITSIZEOF_INT_FAST8_T)/g' \
	    -e 's/@''BITSIZEOF_INT_FAST16_T''@/$(BITSIZEOF_INT_FAST16_T)/g' \
	    -e 's/@''BITSIZEOF_INT_FAST32_T''@/$(BITSIZEOF_INT_FAST32_T)/g' \
	    -e 's/@''BITSIZEOF_INT_FAST64_T''@/$(BITSIZEOF_INT_FAST64_T)/g' \
	    -e 's/@''BITSIZEOF_UINT_FAST8_T''@/$(BITSIZEOF_UINT_FAST8_T)/g' \
	    -e 's/@''BITSIZEOF_UINT_FAST16_T''@/$(BITSIZEOF_UINT_FAST16_T)/g' \
	    -e 's/@''BITSIZEOF_UINT_FAST32_T''@/$(BITSIZEOF_UINT_FAST32_T)/g' \
	    -e 's/@''BITSIZEOF_UINT_FAST64_T''@/$(BITSIZEOF_UINT_FAST64_T)/g' \
	    -e 's/@''BITSIZEOF_INTPTR_T''@/$(BITSIZEOF_INTPTR_T)/g' \
	    -e 's/@''BITSIZEOF_UINTPTR_T''@/$(BITSIZEOF_UINTPTR_T)/g' \
	    -e 's/@''BITSIZEOF_INTMAX_T''@/$(BITSIZEOF_INTMAX_T)/g' \
	    -e 's/@''BITSIZEOF_UINTMAX_T''@/$(BITSIZEOF_UINTMAX_T)/g' \
	    -e 's/@''BITSIZEOF_PTRDIFF_T''@/$(BITSIZEOF_PTRDIFF_T)/g' \
	    -e 's/@''SAME_TYPE_PTRDIFF_T_LONG''@/$(SAME_TYPE_PTRDIFF_T_LONG)/g' \
	    -e 's/@''BITSIZEOF_SIG_ATOMIC_T''@/$(BITSIZEOF_SIG_ATOMIC_T)/g' \
	    -e 's/@''HAVE_SIGNED_SIG_ATOMIC_T''@/$(HAVE_SIGNED_SIG_ATOMIC_T)/g' \
	    -e 's/@''SAME_TYPE_SIG_ATOMIC_T_LONG''@/$(SAME_TYPE_SIG_ATOMIC_T_LONG)/g' \
	    -e 's/@''SAME_TYPE_SIG_ATOMIC_T_UNSIGNED_LONG''@/$(SAME_TYPE_SIG_ATOMIC_T_UNSIGNED_LONG)/g' \
	    -e 's/@''BITSIZEOF_SIZE_T''@/$(BITSIZEOF_SIZE_T)/g' \
	    -e 's/@''SAME_TYPE_SIZE_T_UNSIGNED_LONG''@/$(SAME_TYPE_SIZE_T_UNSIGNED_LONG)/g' \
	    -e 's/@''BITSIZEOF_WCHAR_T''@/$(BITSIZEOF_WCHAR_T)/g' \
	    -e 's/@''HAVE_SIGNED_WCHAR_T''@/$(HAVE_SIGNED_WCHAR_T)/g' \
	    -e 's/@''SAME_TYPE_WCHAR_T_LONG''@/$(SAME_TYPE_WCHAR_T_LONG)/g' \
	    -e 's/@''SAME_TYPE_WCHAR_T_UNSIGNED_LONG''@/$(SAME_TYPE_WCHAR_T_UNSIGNED_LONG)/g' \
	    -e 's/@''BITSIZEOF_WINT_T''@/$(BITSIZEOF_WINT_T)/g' \
	    -e 's/@''HAVE_SIGNED_WINT_T''@/$(HAVE_SIGNED_WINT_T)/g' \
	    -e 's/@''SAME_TYPE_WINT_T_LONG''@/$(SAME_TYPE_WINT_T_LONG)/g' \
	    -e 's/@''SAME_TYPE_WINT_T_UNSIGNED_LONG''@/$(SAME_TYPE_WINT_T_UNSIGNED_LONG)/g' \
	    < $(srcdir)/stdint_.h > $@-t
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

BUILT_SOURCES += $(UNISTD_H)

# We need the following in order to create an empty placeholder for
# <unistd.h> when the system doesn't have one.
unistd.h:
	echo '/* Empty placeholder for $@.  */' >$@
MOSTLYCLEANFILES += unistd.h

libmailutils_la_SOURCES += printf-args.h printf-parse.h vasnprintf.h

libmailutils_la_SOURCES += vsnprintf.h

libmailutils_la_SOURCES += xsize.h

EXTRA_DIST += inttostr.c inttostr.h intprops.h imaxtostr.c offtostr.c umaxtostr.c imaxtostr.c offtostr.c umaxtostr.c getopt.c getopt1.c regex.c fnmatch.c getdelim.c getdelim.h getdelim.c getline.c getline.c getline.h getline.c md5.c md5.h md5.c getndelim2.c mempcpy.c mempcpy.h mempcpy.c strcasecmp.c strncasecmp.c strchrnul.c strndup.c strndup.h strndup.c strnlen.c strnlen.h strnlen.c strtok_r.c vsnprintf.c unlocked-io.h memchr.c glob.c glob_.h glob-libc.h glob.c mbchar.c stat-macros.h strdup.c strdup.h strdup.c sha1.c sha1.h sha1.c vasnprintf.c printf-args.c printf-parse.c asnprintf.c getlogin_r.c getlogin_r.c
