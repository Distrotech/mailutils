dnl This file is part of GNU mailutils.
dnl Copyright (C) 2003, 2007, 2009-2012 Free Software Foundation, Inc.
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

dnl MU_CHECK_GNUTLS([VERSION = `'],[ACTION-IF-TRUE=`'],[ACTION-IF-FALSE=`']
AC_DEFUN([MU_CHECK_GNUTLS],
[
 if test "x$mu_cv_lib_gnutls" = x; then
   cached=""
   AC_ARG_WITH([gnutls],
               AC_HELP_STRING([--without-gnutls],
                              [do not use GNU TLS library]),
               [case "$withval" in
	        yes|no) mu_cv_lib_gnutls=$withval;;
	        *) AC_MSG_ERROR([bad value for --with-gnutls: $withval]);;
                esac],
               [mu_cv_lib_gnutls=yes])

   if test "$mu_cv_lib_gnutls" != "no"; then
     AC_CHECK_HEADER(gnutls/gnutls.h,
                     [:],
                     [mu_cv_lib_gnutls=no])
     if test "$mu_cv_lib_gnutls" != "no"; then
       saved_LIBS=$LIBS
       AC_PREPROC_IFELSE(
         [AC_LANG_PROGRAM([#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#if GNUTLS_VERSION_NUMBER <= 0x020b00
# error "Need gcrypt"
#endif
],[])],
         [],
	 [AC_CHECK_LIB(gcrypt, main,
                       [TLS_LIBS="-lgcrypt"],
                       [mu_cv_lib_gnutls=no])])
       LIBS="$LIBS $TLS_LIBS"
       AC_CHECK_LIB(gnutls, gnutls_global_init,
                    [TLS_LIBS="-lgnutls $TLS_LIBS"],
                    [mu_cv_lib_gnutls=no])
       LIBS=$saved_LIBS
       m4_if([$1],,,[if test $mu_cv_lib_gnutls != no; then
         LIBS="$LIBS $TLS_LIBS"
         AC_TRY_RUN([
#include <gnutls/gnutls.h>

int
main()
{
  return gnutls_check_version ("$1") == (char*) 0;
}],
                    [:],
                    [mu_cv_lib_gnutls=no],
                    [mu_cv_lib_gnutls=no])
         LIBS=$saved_LIBS
       fi])
     fi
   fi
 else
  cached=" (cached) "
 fi
 
 m4_if([$2],,,[if test $mu_cv_lib_gnutls != no; then
   $2
 fi])
 m4_if([$3],,,[if test $mu_cv_lib_gnutls = no; then
   $3
 fi])
 
 AC_MSG_CHECKING([whether to use GNU TLS])
 AC_MSG_RESULT(${cached}${mu_cv_lib_gnutls})])
