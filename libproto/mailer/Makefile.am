## This file is part of GNU Mailutils.
## Copyright (C) 2007-2012 Free Software Foundation, Inc.
##
## GNU Mailutils is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as
## published by the Free Software Foundation; either version 3, or (at
## your option) any later version.
##
## GNU Mailutils is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. 

INCLUDES = @MU_LIB_COMMON_INCLUDES@

lib_LTLIBRARIES = libmu_mailer.la
libmu_mailer_la_LDFLAGS=-version-info @VI_CURRENT@:@VI_REVISION@:@VI_AGE@
libmu_mailer_la_LIBADD = ${MU_LIB_AUTH} ${MU_LIB_MAILUTILS} @INTLLIBS@

if MU_COND_GSASL
  SMTP_GSASL=smtp_gsasl.c
endif
EXTRA_DIST=smtp_gsasl.c

libmu_mailer_la_SOURCES = \
 mbox.c\
 prog.c\
 sendmail.c\
 smtp.c\
 smtp_auth.c\
 smtp_capa.c\
 smtp_capa_itr.c\
 smtp_carrier.c\
 smtp_cmd.c\
 smtp_create.c\
 smtp_data.c\
 smtp_disconnect.c\
 smtp_dot.c\
 smtp_ehlo.c\
 $(SMTP_GSASL)\
 smtp_io.c\
 smtp_mail.c\
 smtp_mech.c\
 smtp_open.c\
 smtp_param.c\
 smtp_quit.c\
 smtp_rcpt.c\
 smtp_rset.c\
 smtp_secret.c\
 smtp_send.c\
 smtp_starttls.c\
 smtp_trace.c\
 smtp_url.c\
 remote.c

