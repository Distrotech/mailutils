dnl This file is part of GNU mailutils.
dnl Copyright (C) 2001, 2005 Free Software Foundation, Inc.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
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

AC_SUBST(MU_LIBOBJS)
AC_SUBST(MU_EXTRA_DIST)

AC_SUBST(MU_EXTRA_DIST)
m4_define([mu_extra_dist],[])
AC_DEFUN([MU_LIBSOURCES],[
m4_foreach(source, [$1], [m4_define([mu_extra_dist], mu_extra_dist source)])])
AC_DEFUN([MU_LIBSOURCE],[m4_define([mu_extra_dist], mu_extra_dist [$1])])

AC_DEFUN([MU_FINISH],[
[MU_EXTRA_DIST]="mu_extra_dist"
])

AC_DEFUN([MU_LIBOBJ],[
 MU_LIBOBJS="$MU_LIBOBJS $1.o"])

AC_DEFUN([MU_CHECK_FUNC],[
 AC_CHECK_FUNC($1,,
      [MU_LIBOBJ($1)])])

AC_DEFUN([MU_REPLACE_FUNCS],
[AC_CHECK_FUNCS([$1], , [dnl
 MU_LIBOBJ($ac_func)
])])

AC_SUBST(GNU_INCLS)
AC_DEFUN([MU_HEADER],[
 pushdef([mu_upcase],translit($1,[a-z-],[A-Z_]))
 pushdef([mu_cache_var],[mu_cv_header_]translit($1,[-./],[___]))

 if test x"[$]mu_cache_var" != xyes; then
   AC_CONFIG_LINKS(include/mailutils/gnu/$1:mailbox/ifelse($2,,$1,$2))
   GNU_INCLS="$GNU_INCLS $1"
   mu_cache_var=yes
 fi

 popdef([mu_upcase])
 popdef([mu_cache_var])
])

