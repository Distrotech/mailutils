AC_DEFUN(MU_CHECK_TLS,
[
 if test "x$WITH_GNUTLS" = x; then
   cached=""
   AC_ARG_WITH([gnutls],
               AC_HELP_STRING([--with-gnutls],
                              [use GNU TLS library]),
               [WITH_GNUTLS=$withval],
               [WITH_GNUTLS=no])

   if test "$WITH_GNUTLS" != "no"; then
     AC_CHECK_HEADER(gnutls/gnutls.h,
                     [:],
                     [WITH_GNUTLS=no])
     if test "$WITH_GNUTLS" != "no"; then
       saved_LIBS=$LIBS
       AC_CHECK_LIB(gnutls, gnutls_global_init,
                    [TLS_LIBS="-lgnutls"],
                    [WITH_GNUTLS=no])
       AC_CHECK_LIB(gcrypt, main,
                    [TLS_LIBS="$TLS_LIBS -lgcrypt"],
                    [WITH_GNUTLS=no])
       LIBS=$saved_LIBS
     fi
   fi
 else
  cached=" (cached) "
 fi
 AC_MSG_CHECKING([whether to use TLS libraries])
 AC_MSG_RESULT(${cached}${WITH_GNUTLS})])
