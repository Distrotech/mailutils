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

EXTRA_DIST = $(TESTSUITE_AT) testsuite package.m4 
DISTCLEANFILES       = atconfig $(check_SCRIPTS)
MAINTAINERCLEANFILES = Makefile.in $(TESTSUITE)

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

AM_CPPFLAGS = @MU_LIB_COMMON_INCLUDES@ 
noinst_PROGRAMS = \
 addr\
 cidr\
 debugspec\
 decode2047\
 encode2047\
 fltst\
 fsaf\
 fsaftomod\
 fsfolder\
 imapio\
 listop\
 mailcap\
 mimehdr\
 modtofsaf\
 msgset\
 modmesg\
 prop\
 scantime\
 strftime\
 strin\
 strout\
 tempfile\
 url-comp\
 url-parse\
 wicket\
 wsp

LDADD = ${MU_LIB_MAILUTILS}

EXTRA_DIST += Encode Decode Wicketfile

## ------------ ##
## Test suite.  ##
## ------------ ##

TESTSUITE_AT = \
 address.at\
 base64d.at\
 base64e.at\
 debugspec.at\
 decode2047.at\
 encode2047.at\
 fromflt.at\
 fsaf.at\
 fsaftomod.at\
 fsfolder00.at\
 fsfolder01.at\
 fsfolder02.at\
 hdrflt.at\
 imapio.at\
 inline-comment.at\
 linecon.at\
 list.at\
 mailcap.at\
 mimehdr.at\
 modmesg00.at\
 modmesg01.at\
 modmesg02.at\
 modmesg03.at\
 modtofsaf.at\
 msgset.at\
 prop.at\
 scantime.at\
 strftime.at\
 strerr.at\
 strin.at\
 strout.at\
 testsuite.at\
 url.at\
 url-comp.at\
 wicket.at\
 wordsplit.at

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


