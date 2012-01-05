# This file is part of GNU Mailutils. -*- Autotest -*-
# Copyright (C) 2007-2012 Free Software Foundation, Inc.
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
# This file is part of Mailfromd testsuite. 

# Require a reasonably recent autotest.
m4_version_prereq([2.52g])

m4_define([AT_SKIP_TEST],[exit 77])

dnl ------------------------------------------------------------
dnl MUT_VERSION(TOOL)  -- checks the version output from TOOL
dnl
m4_define([MUT_VERSION],[
AT_SETUP([$1 version])

AT_CHECK([$1 --version | sed '1{s/ *[\[.*\]]//;q;}' ],
         [0],
         [$1 (AT_PACKAGE_NAME) AT_PACKAGE_VERSION
],
[],
[cat >.xfailfile <<'_EOT'

==============================================================
WARNING: Not using the proper version, *all* checks dubious...
==============================================================
_EOT
],
[rm -f $[]XFAILFILE])

AT_CLEANUP
])

dnl ------------------------------------------------------------
dnl MUT_MBCHMOD(MBOX, MODE)
dnl
m4_define([MUT_MBCHMOD],[
# First, set all modes to $2
find $1 | xargs chmod $2
# Then, remove executable bit from regular files
find $1 -type f | xargs chmod -x
])

dnl ------------------------------------------------------------
dnl MUT_MBCOPY(SRC, [DST = `.'], [mode])  -- Copy mailbox SRC to DST.
dnl 
dnl
m4_define([MUT_MBCOPY],[
m4_pushdef([__dst],[m4_if([$2],,[.],[$2])])
m4_pushdef([__basename],[m4_bregexp($1,[.*/\([^/]+\)$],\1)])
m4_pushdef([__mode],[m4_if([$3],,[+w],[$3])])
cp -r $1 __dst
if test -e __dst/__basename; then
 MUT_MBCHMOD(__dst/__basename, __mode)
else
 MUT_MBCHMOD(__dst, __mode)
fi
m4_popdef([__basename])
m4_popdef([__dst])
])
