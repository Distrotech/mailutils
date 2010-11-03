## This file is part of GNU Mailutils.
## Copyright (C) 1999, 2000, 2001, 2002, 2005, 2007, 2010 Free
## Software Foundation, Inc.
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
dist_bin_SCRIPTS = mailutils-config

if MU_COND_SUPPORT_POP
 POP_C=pop.c
endif

EXTRA_DIST=pop.c mu-setup.h mu-setup.c mu-setup.awk
BUILT_SOURCES=mu-setup.h mu-setup.c

MODULES = \
 acl.c\
 cflags.c\
 filter.c\
 flt2047.c\
 help.c\
 info.c\
 ldflags.c\
 $(POP_C)\
 query.c\
 wicket.c

mu_SOURCES = \
 dispatch.c\
 mu.h\
 mu.c\
 shell.c\
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
 @READLINE_LIBS@ @MU_COMMON_LIBRARIES@

INCLUDES = @MU_APP_COMMON_INCLUDES@ @MU_AUTHINCS@
AM_CPPFLAGS = \
  -DCOMPILE_FLAGS="\"-I$(includedir)\"" \
  -DLINK_FLAGS="\"-L$(libdir)\"" \
  -DLINK_POSTFLAGS="\"$(MU_LINK_POSTFLAGS)\"" \
  -DAUTHLIBS="\"$(MU_AUTHLIBS)\"" \
  -DGUILE_LIBS="\"$(GUILE_LIBS)\"" \
  -DPYTHON_LIBS="\"$(PYTHON_LIBS)\"" \
  -DI18NLIBS="\"$(LIBINTL)\""

mu-setup.h: Makefile.am $(MODULES)
	$(AM_V_GEN)$(AWK) -f $(top_srcdir)/mu/mu-setup.awk -v mode=h $(MODULES) > mu-setup.h

mu-setup.c: Makefile.am $(MODULES)
	$(AM_V_GEN)$(AWK) -f $(top_srcdir)/mu/mu-setup.awk -v mode=c $(MODULES) > mu-setup.c
