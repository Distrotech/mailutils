dnl This file is part of GNU mailutils.
dnl Copyright (C) 2001 Free Software Foundation, Inc.
dnl
dnl This file is free software; as a special exception the author gives
dnl unlimited permission to copy and/or distribute it, with or without
dnl modifications, as long as this notice is preserved.
dnl
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
dnl implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
dnl
dnl Check for --enable-maintainer-mode and enable maintainer-specific
dnl settings.

AC_DEFUN(MU_MAINTAINER_MODE,
  [AM_MAINTAINER_MODE
   if test x"$USE_MAINTAINER_MODE" = xyes; then
     if test x"$GCC" = xyes; then
       AC_MSG_CHECKING(whether gcc accepts -ggdb)
       CFLAGS="-ggdb"
       AC_TRY_COMPILE([],void f(){},
                      AC_MSG_RESULT(yes),
		      [if test x"$ac_cv_prog_cc_g" = xyes; then
                         CFLAGS=-g
                       else
                         CFLAGS=
                       fi
		       AC_MSG_RESULT(no)])
       CFLAGS="$CFLAGS -DMAINTAINER_MODE"
     fi
   fi])
