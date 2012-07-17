dnl This file is part of GNU mailutils.
dnl Copyright (C) 2002, 2005, 2007, 2009-2012 Free Software Foundation,
dnl Inc.
dnl
dnl GNU Mailutils is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl GNU Mailutils is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.

dnl MU_ENABLE_SUPPORT(feature, [action-if-true], [action-if-false],
dnl                   [additional-cond], [default-value])

AC_DEFUN([MU_ENABLE_SUPPORT], [
	pushdef([mu_upcase],translit($1,[a-z+-],[A-ZX_]))
	pushdef([mu_cache_var],[mu_cv_enable_]translit($1,[+-],[x_]))
	pushdef([mu_cond],[MU_COND_SUPPORT_]mu_upcase)

       	ifelse([$4],,,[if test $4; then])
	  AC_ARG_ENABLE($1, 
	                AC_HELP_STRING([--disable-]$1,
		                       [disable ]$1[ support]),
	                [
	  case "${enableval}" in
		yes) mu_cache_var=yes;;
                no)  mu_cache_var=no;;
	        *)   AC_MSG_ERROR([bad value ${enableval} for --disable-$1]) ;;
          esac],
                      [mu_cache_var=ifelse([$5],,yes,[$5])])

	  if test "[$]mu_cache_var" = "yes"; then
		  ifelse([$2],,:,[$2])
 	  ifelse([$3],,,else
                 [$3])
	  fi
	ifelse([$4],,,[else
		mu_cache_var=no
        fi])
	if test "[$]mu_cache_var" = "yes"; then
		AC_DEFINE([ENABLE_]mu_upcase,1,[Define this if you enable $1 support])
        fi
	AM_CONDITIONAL(mu_cond, [test "[$]mu_cache_var" = "yes"])
	popdef([mu_cond])
	popdef([mu_cache_var])
	popdef([mu_upcase])
])

dnl MU_ENABLE_BUILD(feature, [action-if-true], [action-if-false],
dnl                 [additional-cond], [default-value], [listvar])
AC_DEFUN([MU_ENABLE_BUILD], [
	pushdef([mu_upcase],translit($1,[a-z+-],[A-ZX_]))
	pushdef([mu_cache_var],[mu_cv_enable_build_]translit($1,[+-],[x_]))
	pushdef([mu_cond],[MU_COND_]mu_upcase)

	AC_ARG_ENABLE(build-$1, 
	              AC_HELP_STRING([--disable-build-]$1,
	                             [do not build ]$1),
	                [
  	  case "${enableval}" in
		yes) mu_cache_var=yes;;
                no)  mu_cache_var=no;;
	        *)   AC_MSG_ERROR([bad value ${enableval} for --disable-$1]) ;;
          esac],
                      [ifelse([$4],,[mu_cache_var=ifelse([$5],,yes,[$5])],
		              [if test $4; then
			         mu_cache_var=yes
			       else
			         mu_cache_var=no
			       fi])])

	if test "[$]mu_cache_var" = "yes"; then
		  ifelse([$2],,:,[$2])
		  ifelse([$6],,,[$6="$[]$6 $1"])
	ifelse([$3],,,else
               [$3])
	fi
	if test "[$]mu_cache_var" = "yes"; then
	  AC_DEFINE([MU_BUILD_]mu_upcase,1,[Define this if you build $1])
        fi
	  
	AM_CONDITIONAL(mu_cond,
	               [test "[$]mu_cache_var" = "yes"])
	
	popdef([mu_upcase])
	popdef([mu_cache_var])
	popdef([mu_cond])
])


