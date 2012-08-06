## This file is part of GNU Mailutils.
## Copyright (C) 1999-2002, 2005, 2007, 2010-2012 Free Software
## Foundation, Inc.
##
## GNU Mailutils is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as
## published by the Free Software Foundation; either version 2, or (at
## your option) any later version.
##
## GNU Mailutils is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.

bin_PROGRAMS = mu
bin_SCRIPTS = mailutils-config

CLEANFILES=mailutils-config
mailutils-config: $(top_srcdir)/mu/mailutils-config.in
	$(AM_V_GEN)muname=`echo mu | \
              sed -e 's/$(EXEEXT)$$//;$(transform);s/$$/$(EXEEXT)/' | \
              sed 'N;N;N;s,\n, ,g'`; \
	sed 's/MUNAME/'$$muname'/g' $(top_srcdir)/mu/mailutils-config.in > mailutils-config


IDLE_MODULES=

if MU_COND_SUPPORT_POP
 POP_C=pop.c
else
 IDLE_MODULES+=pop.c
endif

if MU_COND_SUPPORT_IMAP
 IMAP_C=imap.c
else
 IDLE_MODULES+=imap.c
endif

if MU_COND_DBM
 DBM_C=dbm.c
 LIBMU_DBM=../libmu_dbm/libmu_dbm.la
else
 IDLE_MODULES+=dbm.c
endif

if MU_COND_SUPPORT_SMTP
 SMTP_C=smtp.c
else
 IDLE_MODULES+=smtp.c
endif

MODULES = \
 acl.c\
 cflags.c\
 $(DBM_C)\
 $(IMAP_C)\
 filter.c\
 flt2047.c\
 help.c\
 info.c\
 ldflags.c\
 logger.c\
 $(POP_C)\
 query.c\
 send.c\
 $(SMTP_C)\
 wicket.c

mu_SOURCES = \
 dispatch.c\
 getans.c\
 getarg.c\
 getyn.c\
 mu.h\
 mu.c\
 shell.c\
 util.c\
 verbose.c\
 $(MODULES)

mu_LDADD = \
 ${MU_APP_LIBRARIES}\
 ${MU_LIB_MBOX}\
 ${MU_LIB_IMAP}\
 ${MU_LIB_POP}\
 ${MU_LIB_NNTP}\
 ${MU_LIB_MH}\
 ${MU_LIB_MAILDIR}\
 ${MU_LIB_MAILER}\
 ${MU_LIB_AUTH}\
 @MU_AUTHLIBS@\
 ${MU_LIB_MAILUTILS}\
 ${LIBMU_DBM} @DBMLIBS@\
 @READLINE_LIBS@ @MU_COMMON_LIBRARIES@

INCLUDES = @MU_APP_COMMON_INCLUDES@ @MU_AUTHINCS@
AM_CPPFLAGS = \
  -DCOMPILE_FLAGS="\"-I$(includedir)\"" \
  -DLINK_FLAGS="\"-L$(libdir)\"" \
  -DLINK_POSTFLAGS="\"$(MU_LINK_POSTFLAGS)\"" \
  -DAUTHLIBS="\"$(MU_AUTHLIBS)\"" \
  -DGUILE_LIBS="\"$(GUILE_LIBS)\"" \
  -DPYTHON_LIBS="\"$(PYTHON_LIBS)\"" \
  -DI18NLIBS="\"$(LIBINTL)\"" \
  -DDBMLIBS="\"$(DBMLIBS)\""

BUILT_SOURCES=mu-setup.c mu-setup.h
EXTRA_DIST=mu-setup.awk mu-setup.c mu-setup.h template.c mailutils-config.in

mu-setup.h: Makefile.am $(MODULES) $(IDLE_MODULES) 
	$(AM_V_GEN)$(AWK) -f $(top_srcdir)/mu/mu-setup.awk -v mode=h \
           $(MODULES) $(IDLE_MODULES) > mu-setup.h

mu-setup.c: Makefile.am $(MODULES)
	$(AM_V_GEN)$(AWK) -f $(top_srcdir)/mu/mu-setup.awk -v mode=c \
           $(MODULES) $(IDLE_MODULES) > mu-setup.c
