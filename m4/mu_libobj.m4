dnl This file is part of GNU mailutils.
dnl Copyright (C) 2001 Free Software Foundation, Inc.
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
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
dnl

AC_SUBST(MU_LIBOBJS)
AC_DEFUN(MU_LIBOBJ,[
 MU_LIBOBJS="$MU_LIBOBJS $1.o"])

AC_DEFUN(MU_CHECK_FUNC,[
 AC_CHECK_FUNC($1,,
      [MU_LIBOBJ($1)])])

AC_DEFUN(MU_CHECK_FUNCS,[
 for mu_func in $1
 do
   MU_CHECK_FUNC($mu_func)
 done])

AC_SUBST(GNU_INCLS)
AC_DEFUN(MU_HEADER,[
 mu_header_list="$mu_header_list include/mailutils/gnu/$1:headers/$1"
 GNU_INCLS="$GNU_INCLS $1"
])

AC_DEFUN(MU_FINISH_LINKS,[
 AC_CONFIG_LINKS([$mu_header_list])
])