## This file is part of GNU Mailutils.
## Copyright (C) 1999-2003, 2005, 2007, 2010-2012 Free Software
## Foundation, Inc.
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

INCLUDES = @MU_APP_COMMON_INCLUDES@ 

sbin_PROGRAMS = pop3d
bin_PROGRAMS = @POPAUTH@
EXTRA_PROGRAMS = popauth
SUBDIRS = testsuite

pop3d_SOURCES =\
 apop.c\
 auth.c\
 bulletin.c\
 capa.c\
 cmd.c\
 dele.c\
 expire.c\
 extra.c\
 list.c\
 logindelay.c\
 noop.c\
 pop3d.c\
 pop3d.h\
 quit.c\
 retr.c\
 rset.c\
 signal.c\
 stat.c\
 stls.c\
 top.c\
 uidl.c\
 user.c

if MU_COND_DBM
  LIBMU_DBM=../libmu_dbm/libmu_dbm.la
endif

pop3d_LDADD = \
 ${MU_APP_LIBRARIES}\
 ${MU_LIB_MBOX}\
 ${MU_LIB_MH}\
 ${MU_LIB_MAILDIR}\
 ${MU_LIB_AUTH}\
 @MU_AUTHLIBS@ \
 ${MU_LIB_MAILUTILS}\
 @MU_COMMON_LIBRARIES@\
 ${LIBMU_DBM}\
 @DBMLIBS@\
 @TCPWRAP_LIBRARIES@

popauth_SOURCES = popauth.c
popauth_LDADD = \
 ${MU_APP_LIBRARIES}\
 ${MU_LIB_MAILUTILS}\
 @MU_COMMON_LIBRARIES@\
 ${LIBMU_DBM}\
 @DBMLIBS@

pop3d_DEPENDENCIES = \
 @MU_AUTHLIBS_DEPENDENCY@ \
 ../lib/libmuaux.a \
 ${MU_LIB_MBOX}\
 ${MU_LIB_MH}\
 ${MU_LIB_MAILDIR}\
 ${MU_LIB_MAILUTILS}\
 ${LIBMU_DBM}

