dnl Copyright (C) 2008, 2010, 2013 Free Software Foundation, Inc.
dnl GNU Radius is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl GNU Radius is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

dnl AM_GNU_RADIUS(minversion, [act-if-found], [ac-if-not-found])
dnl                  $1             $2               $3
dnl Verify if GNU Radius is installed and if its version is `minversion'
dnl or newer.  If not installed, execute `ac-if-not-found' or, if it is not
dnl given, spit out an error message.
dnl If GNU Radius is found, set:
dnl     GNU_RADIUS_VERSION to the Radius version (string);
dnl     GNU_RADIUS_VERSION_NUMBER to the packed numeric representation of the
dnl         GNU Radius version (major * 1000 + minor * 100 + patch);
dnl     GNU_RADIUS_LIBS to the list of cc(1) flags needed to link in the
dnl         GNU Radius libraries
dnl
dnl Finally, if `act-if-found' is given, execute it.  Otherwise, append the
dnl value of $GNU_RADIUS_LIBS to LIBS. 

AC_DEFUN([_AM_GRAD_PREPROC],[
AC_REQUIRE_CPP()dnl
AC_LANG_CONFTEST([$1])
$3=`$CPP conftest.$ac_ext 2>conftest.err | sed -n 's/^$2//p'`
rm -f conftest.err conftest.$ac_ext])

m4_define([grad_version_number],[dnl
  patsubst([$1],[\([0-9]+\)\.\([0-9]+\)\(\.\([0-9]+\)\)?.*],
           [m4_builtin(eval, \1*1000 + \2*100 + \40 / 10)])dnl
])

AC_DEFUN([AM_GNU_RADIUS],
  [AC_SUBST(GNU_RADIUS_VERSION)
   AC_SUBST(GNU_RADIUS_VERSION_NUMBER)
   AC_SUBST(GNU_RADIUS_LIBS)

   AC_CHECK_HEADERS([radius/radius.h],
                    [grad_cv_radius=yes],
                    [grad_cv_radius=no])
   if test $grad_cv_radius = yes; then
     AC_CHECK_LIB(gnuradius, grad_dict_init,
                  :,
                  [grad_cv_radius=no])
   fi
   if test $grad_cv_radius = yes; then
     _AM_GRAD_PREPROC([AC_LANG_PROGRAM([
#include <radius/types.h>
version=GRAD_VERSION_MAJOR.GRAD_VERSION_MINOR.GRAD_VERSION_PATCH
],[])],
      [version=],
      [grad_version_string])          
  
     GNU_RADIUS_VERSION=
     for f in $grad_version_string
     do
       GNU_RADIUS_VERSION="${GNU_RADIUS_VERSION}$f"
     done
     if test -n "GNU_RADIUS_VERSION"; then
        VEX=`echo $GNU_RADIUS_VERSION | sed 's/\./ \\\\* 1000 + /;s/\./ \\\\* 100 + /'`
        GNU_RADIUS_VERSION_NUMBER=`eval expr "$VEX"`

        ifelse($1,,,[
          if test $GNU_RADIUS_VERSION_NUMBER -lt grad_version_number($1); then
	    ifelse($3,,
                   [AC_MSG_ERROR([GNU Radius version too old; required is at least ]$1)],
		   [$3])
          fi])
     
        GNU_RADIUS_LIBS="-lgnuradius"
        ifelse($2,,[LIBS="$LIBS $GNU_RADIUS_LIBS"], [$2])
     fi
   fi
   if test $grad_cv_radius != yes; then
     ifelse($3,,[AC_MSG_ERROR(cannot find GNU Radius)], [$3])
   fi])
 
