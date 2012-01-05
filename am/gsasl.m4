dnl This file is part of GNU mailutils.
dnl Copyright (C) 2003, 2007, 2010-2012 Free Software Foundation, Inc.
dnl
dnl GNU Mailutils is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3, or (at your option)
dnl any later version.
dnl 
dnl GNU Mailutils is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License along
dnl with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.
dnl

dnl MU_CHECK_GSASL([VERSION = `'],[ACTION-IF-TRUE=`'],[ACTION-IF-FALSE=`']
AC_DEFUN([MU_CHECK_GSASL],
[if test "x$mu_cv_lib_gsasl" = x; then
   cached=""
   mu_cv_lib_gsasl=no

   AC_ARG_WITH(gsasl,
     AC_HELP_STRING([--without-gsasl], [do not use libgsasl]),
     [case $withval in
      yes|no) wantgsasl=$withval;;
      *) AC_MSG_ERROR([bad value for --with-gsasl: $withval]);;
      esac],
     [wantgsasl=yes])

   if test $wantgsasl = yes; then
     AC_CHECK_HEADER(gsasl.h,
                     [:],
                     [wantgsasl=no])

     if test $wantgsasl != no; then
       save_LIBS=$LIBS
       AC_CHECK_LIB(gsasl, gsasl_init,
                    [mu_cv_lib_gsasl=-lgsasl],
                    [mu_cv_lib_gsasl=no])
       if test $mu_cv_lib_gsasl != no; then
         LIBS="$LIBS $mu_cv_lib_gsasl"
         AC_TRY_RUN([
#include <gsasl.h>

int
main()
{
  return gsasl_check_version ("$1") == (char*) 0;
}],
                    [:],
                    [mu_cv_lib_gsasl=no],
                    [mu_cv_lib_gsasl=no])
       fi
       LIBS=$save_LIBS
     fi       
   fi
 else
  cached=" (cached) "
 fi
 if test $mu_cv_lib_gsasl != no; then
   GSASL_LIBS=$mu_cv_lib_gsasl
   m4_if([$2],,,[$2])
 fi
 m4_if([$3],,,[if test $mu_cv_lib_gsasl = no; then
   $3
 fi])
 AC_MSG_CHECKING([whether to use GSASL])
 AC_MSG_RESULT(${cached}${mu_cv_lib_gsasl})
 ])
 
