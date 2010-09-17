## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
##
##   GNU Mailutils is free software; you can redistribute it and/or
##   modify it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 3, or (at
##   your option) any later version.
##
##   This program is distributed in the hope that it will be useful, but
##   WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##   General Public License for more details.
##
##   You should have received a copy of the GNU General Public License
##   along with this program; if not, write to the Free Software
##   Foundation, Inc.  51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

INCLUDES = @MU_LIB_COMMON_INCLUDES@

lib_LTLIBRARIES = libmu_mailer.la
libmu_mailer_la_LDFLAGS=-version-info @VI_CURRENT@:@VI_REVISION@:@VI_AGE@
libmu_mailer_la_LIBADD = ${MU_LIB_AUTH} ${MU_LIB_MAILUTILS} @INTLLIBS@

libmu_mailer_la_SOURCES = \
 mbox.c\
 prog.c\
 sendmail.c\
 smtp.c\
 smtp_capa.c\
 smtp_carrier.c\
 smtp_create.c\
 smtp_data.c\
 smtp_disconnect.c\
 smtp_domain.c\
 smtp_ehlo.c\
 smtp_io.c\
 smtp_mail.c\
 smtp_rcpt.c\
 smtp_open.c\
 smtp_rset.c\
 smtp_starttls.c\
 smtp_trace.c\
 smtp_quit.c\
 remote.c

