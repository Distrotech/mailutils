# Based on gssapi.m4 by Brendan Cully <brendan@kublai.com> 20010529

dnl MU_CHECK_GSSAPI(PREFIX)
dnl Search for a GSSAPI implementation in the standard locations plus PREFIX,
dnl if it is set and not "yes".
dnl Defines GSSAPI_CFLAGS and GSSAPI_LIBS if found.
dnl Defines GSSAPI_IMPL to "Heimdal", "MIT", or "OldMIT", or "none" if not found
AC_DEFUN(MU_CHECK_GSSAPI,
[
 if test "x$mu_cv_lib_gssapi_libs" = x; then
  cache=""
  GSSAPI_PREFIX=[$1]
  GSSAPI_IMPL="none"
  # First try krb5-config
  if test "$GSSAPI_PREFIX" != "yes"; then
    krb5_path="$GSSAPI_PREFIX/bin"
  else
    krb5_path="$PATH"
  fi
  AC_PATH_PROG(KRB5CFGPATH, krb5-config, none, $krb5_path)
  if test "$KRB5CFGPATH" != "none"; then
    GSSAPI_CFLAGS="$CPPFLAGS `$KRB5CFGPATH --cflags gssapi`"
    GSSAPI_LIBS="$MUTTLIBS `$KRB5CFGPATH --libs gssapi`"
    GSSAPI_IMPL="Heimdal"
  else
    ## OK, try the old code
    saved_CPPFLAGS="$CPPFLAGS"
    saved_LDFLAGS="$LDFLAGS"
    saved_LIBS="$LIBS"
    if test "$GSSAPI_PREFIX" != "yes"; then
      GSSAPI_CFLAGS="-I$GSSAPI_PREFIX/include"
      GSSAPI_LDFLAGS="-L$GSSAPI_PREFIX/lib"
      CPPFLAGS="$CPPFLAGS $GSSAPI_CFLAGS"
      LDFLAGS="$LDFLAGS $GSSAPI_LDFLAGS"
    fi

    ## Check for new MIT kerberos V support
    AC_CHECK_LIB(gssapi_krb5, gss_init_sec_context,
      [GSSAPI_IMPL="MIT",
       GSSAPI_LIBS="$GSSAPI_LDFLAGS -lgssapi_krb5 -lkrb5 -lk5crypto -lcom_err"]
       ,, -lkrb5 -lk5crypto -lcom_err)

    ## Heimdal kerberos V support
    if test "$GSSAPI_IMPL" = "none"; then
      AC_CHECK_LIB(gssapi, gss_init_sec_context,
        [GSSAPI_IMPL="Heimdal"
         GSSAPI_LDFLAGS="$GSSAPI_LDFLAGS -lgssapi -lkrb5 -ldes -lasn1 -lroken"
         GSSAPI_LIBS="$GSSAPI_LDFLAGS -lcrypt -lcom_err"]
         ,, -lkrb5 -ldes -lasn1 -lroken -lcrypt -lcom_err)
    fi

    ## Old MIT Kerberos V
    ## Note: older krb5 distributions use -lcrypto instead of
    ## -lk5crypto, which collides with OpenSSL.  One way of dealing
    ## with that is to extract all objects from krb5's libcrypto
    ## and from openssl's libcrypto into the same directory, then
    ## to create a new libcrypto from these.
    if test "$GSSAPI_IMPL" = "none"; then
      AC_CHECK_LIB(gssapi_krb5, g_order_init,
        [GSSAPI_IMPL="OldMIT",
        GSSAPI_LIBS="$GSSAPI_LDFLAGS -lgssapi_krb5 -lkrb5 -lcrypto -lcom_err"]
        ,, -lkrb5 -lcrypto -lcom_err)
    fi
  fi

  mu_cv_lib_gssapi_cflags="$GSSAPI_CFLAGS"
  mu_cv_lib_gssapi_libs="$GSSAPI_LIBS"
  mu_cv_lib_gssapi_impl="$GSSAPI_IMPL"

  LDFLAGS="$saved_LDFLAGS"
  LIBS="$saved_LIBS"
 else
  cached=" (cached) "
  GSSAPI_CFLAGS="$mu_cv_lib_gssapi_cflags"
  GSSAPI_LIBS="$mu_cv_lib_gssapi_libs"
  GSSAPI_IMPL="$mu_cv_lib_gssapi_impl"
 fi
 AC_MSG_CHECKING(GSSAPI implementation)
 AC_MSG_RESULT(${cached}$GSSAPI_IMPL)
])
