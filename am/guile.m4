dnl This file is part of GNU mailutils.
dnl Copyright (C) 2001, 2006, 2007 Free Software Foundation, Inc.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software Foundation,
dnl Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
dnl

dnl MU_CHECK_GUILE(minversion, [act-if-found], [ac-if-not-found])
dnl                    $1            $2               $3
AC_DEFUN([MU_CHECK_GUILE],
[
  AS_VAR_SET([mu_cv_guile], [no])
  AC_PATH_PROG(GUILE_CONFIG, guile-config, no, $PATH)
  if test "$GUILE_CONFIG" = no; then
    m4_if($3,,[AC_MSG_ERROR(cannot find Guile)], [$3])
  else
    AC_SUBST(GUILE_INCLUDES)
    AC_SUBST(GUILE_LIBS)
    AC_SUBST(GUILE_VERSION)
    AC_SUBST(GUILE_VERSION_NUMBER)
  
    GUILE_INCLUDES=`$GUILE_CONFIG compile`
    GUILE_LIBS=`$GUILE_CONFIG link`
    GUILE_VERSION=`($GUILE_CONFIG --version 2>&1; echo '')|sed 's/guile-config [[^0-9]]* \([[0-9]][[0-9.]]*\)$/\1/'`
    VEX=`echo $GUILE_VERSION | sed 's/\./ \\\\* 1000 + /;s/\./ \\\\* 100 + /'`
    GUILE_VERSION_NUMBER=`eval expr "$VEX"`

    ifelse($1,,,[
      VEX=`echo $1 | sed 's/\./ \\\\* 1000 + /;s/\./ \\\\* 100 + /'`
      min=`eval expr "$VEX"`
      if test $GUILE_VERSION_NUMBER -lt $min; then
        m4_if($3,,
	        [AC_MSG_ERROR([Guile version too old; required is at least ]$1)],
	        [$3])
      fi])

    save_LIBS=$LIBS
    save_CFLAGS=$CFLAGS
    LIBS="$LIBS $GUILE_LIBS"
    CFLAGS="$CFLAGS $GUILE_INCLUDES"
    AC_TRY_LINK([#include <libguile.h>],
                 m4_if([$1], , scm_shell(0, NULL);, [$1]),
                [AS_VAR_SET([mu_cv_guile], $GUILE_VERSION)])
    LIBS=$save_LIBS
    CFLAGS=$save_CFLAGS
  fi

  if test $mu_cv_guile = no; then
    GUILE_INCLUDES=
    GUILE_LIBS=
    GUILE_VERSION=
    GUILE_VERSION_NUMBER=
    m4_if($3,,[AC_MSG_ERROR(required library libguile not found)], [$3])
  else    
    AC_DEFINE_UNQUOTED(GUILE_VERSION, "$GUILE_VERSION",
                       [Guile version number])
    AC_DEFINE_UNQUOTED(GUILE_VERSION_NUMBER, $GUILE_VERSION_NUMBER,
                       [Guile version number: MAX*10 + MIN])
    m4_if($2,,,[$2])
  fi
])     

