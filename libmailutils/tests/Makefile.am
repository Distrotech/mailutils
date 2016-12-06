# This file is part of GNU Mailutils.
# Copyright (C) 2010-2012, 2014-2016 Free Software Foundation, Inc.
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
 globtest\
 imapio\
 listop\
 mailcap\
 mimehdr\
 modtofsaf\
 msgset\
 modmesg\
 parseopt\
 prop\
 scantime\
 strftime\
 strin\
 strout\
 strtoc\
 tempfile\
 tcli\
 url-comp\
 url-parse\
 wicket\
 wordwrap\
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
 htmlent.at\
 globtest.at\
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
 parseopt00.at\
 parseopt01.at\
 parseopt02.at\
 parseopt03.at\
 parseopt04.at\
 parseopt05.at\
 parseopt06.at\
 parseopt07.at\
 parseopt08.at\
 parseopt09.at\
 parseopt10.at\
 parseopt11.at\
 parseopt12.at\
 parseopt13.at\
 parseopt14.at\
 parseopt15.at\
 parseopt16.at\
 parseopt17.at\
 parseopt18.at\
 parseopt19.at\
 parseopt20.at\
 parseopt21.at\
 parseopt22.at\
 parseopt23.at\
 parseopt24.at\
 parseopt25.at\
 parseopt_help00.at\
 parseopt_help01.at\
 parseopt_help02.at\
 parseopt_help03.at\
 parseopt_help04.at\
 parseopt_help05.at\
 parseopt_help06.at\
 parseopt_help07.at\
 parseopt_help08.at\
 parseopt_help09.at\
 parseopt_help10.at\
 parseopt_help11.at\
 parseopt_help12.at\
 prop.at\
 scantime.at\
 strftime.at\
 strerr.at\
 strin.at\
 strout.at\
 strtoc.at\
 testsuite.at\
 url.at\
 url-comp.at\
 xml.at\
 wicket.at\
 wordsplit.at\
 wordwrap00.at\
 wordwrap01.at\
 wordwrap02.at\
 wordwrap03.at

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


