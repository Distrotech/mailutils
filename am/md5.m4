# md5.m4 serial 9
dnl Copyright (C) 2002-2006, 2010-2012 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_MD5],
[
  MU_LIBOBJ([md5])

  dnl Prerequisites of lib/md5.c.
  AC_REQUIRE([AC_C_BIGENDIAN])
  :
])
