# Copyright (C) 2007, 2009-2012 Free Software Foundation, Inc.
#
# GNU Mailutils is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3, or (at
# your option) any later version.
#
# GNU Mailutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.

SUBDIRS = . tests

sbin_PROGRAMS=maidag
maidag_SOURCES=\
 deliver.c\
 forward.c\
 guile.c\
 lmtp.c\
 maidag.c\
 maidag.h\
 mailquota.c\
 python.c\
 sieve.c\
 script.c\
 util.c

if MU_COND_DBM
 LIBMU_DBM=../libmu_dbm/libmu_dbm.la
endif

maidag_LDADD = \
 @LIBMU_SCM@ @GUILE_LIBS@\
 @LIBMU_SCM_DEPS@\
 @MU_LIB_PY@ @PYTHON_LIBS@\
 ${MU_APP_LIBRARIES}\
 ${MU_LIB_SIEVE}\
 ${MU_LIB_MBOX}\
 ${MU_LIB_IMAP}\
 ${MU_LIB_POP}\
 ${MU_LIB_NNTP}\
 ${MU_LIB_MH}\
 ${MU_LIB_MAILDIR}\
 ${MU_LIB_AUTH}\
 ${MU_LIB_MAILER}\
 @MU_AUTHLIBS@\
 ${MU_LIB_MAILUTILS} \
 @MU_COMMON_LIBRARIES@\
 $(LIBMU_DBM)\
 @DBMLIBS@\
 @TCPWRAP_LIBRARIES@

INCLUDES = -I${top_srcdir} @MU_APP_COMMON_INCLUDES@ @GUILE_INCLUDES@ \
 @PYTHON_INCLUDES@

install-exec-hook:
	for i in $(sbin_PROGRAMS); do\
		chown root:mail $(DESTDIR)$(sbindir)/$$i;\
		chmod 4755 $(DESTDIR)$(sbindir)/$$i;\
	done
