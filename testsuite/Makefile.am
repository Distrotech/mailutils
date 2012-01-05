# This file is part of GNU Mailutils.
# Copyright (C) 2010-2012 Free Software Foundation, Inc.
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

DISTCLEANFILES       = atconfig $(check_SCRIPTS)
MAINTAINERCLEANFILES = Makefile.in $(TESTSUITE)
EXTRA_DIST = \
 $(TESTSUITE_AT)\
 testsuite\
 testsuite.inc\
 package.m4\
 lib\
 etc\
 spool\
 folder\
 maildir\
 mh 

## ------------ ##
## package.m4.  ##
## ------------ ##

$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	$(AM_V_GEN){                                      \
	  echo '# Signature of the current package.'; \
	  echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])'; \
	  echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])'; \
	  echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])'; \
	  echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])'; \
	  echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	} >$(srcdir)/package.m4

#

## -------------------------- ##
## Non-installable programs
## -------------------------- ##

INCLUDES = @MU_LIB_COMMON_INCLUDES@ 
noinst_PROGRAMS = \
 bs\
 fldel\
 lstuid\
 mbdel\
 msgset\
 mimetest\
 smtpsend\
 ufms

LDADD = \
 ${MU_LIB_MBOX}\
 ${MU_LIB_IMAP}\
 ${MU_LIB_POP}\
 ${MU_LIB_NNTP}\
 ${MU_LIB_MH}\
 ${MU_LIB_MAILDIR}\
 ${MU_LIB_MAILER}\
 ${MU_LIB_AUTH}\
 @MU_AUTHLIBS@\
 ${MU_LIB_MAILUTILS}

smtpsend_LDADD = \
 ${MU_LIB_MAILER} \
 @MU_AUTHLIBS@\
 ${MU_LIB_MAILUTILS}

## ------------ ##
## Test suite.  ##
## ------------ ##

TESTSUITE_AT = \
 fldel.at\
 lstuid00.at\
 lstuid01.at\
 lstuid02.at\
 mbdel.at\
 mime.at\
 smtp-msg.at\
 smtp-str.at\
 ufms.at\
 testsuite.at

TESTSUITE = $(srcdir)/testsuite
M4=m4

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): package.m4 $(TESTSUITE_AT) $(top_srcdir)/testsuite/testsuite.inc
	$(AM_V_GEN)$(AUTOTEST) -I $(srcdir) -I $(top_srcdir)/testsuite testsuite.at -o $@.tmp
	$(AM_V_at)mv $@.tmp $@

atconfig: $(top_builddir)/config.status
	cd $(top_builddir) && ./config.status tests/$@

clean-local:
	@test ! -f $(TESTSUITE) || $(SHELL) $(TESTSUITE) --clean

check-local: atconfig atlocal $(TESTSUITE) 
	@$(SHELL) $(TESTSUITE)

# Run the test suite on the *installed* tree.
#installcheck-local:
#	$(SHELL) $(TESTSUITE) AUTOTEST_PATH=$(exec_prefix)/bin


