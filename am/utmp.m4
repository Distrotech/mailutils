dnl MU_CHECK_UTMP -- Check for the presence of utmp.h, setutent, getutent
dnl                  and endutent calls.
AC_DEFUN([MU_CHECK_UTMP],
[
  AC_CHECK_HEADERS([utmp.h utmpx.h])
  # The three functions setutent,getutent and endutent depend on each other,
  # so it seems reasonable to provide replacements for all three if getutent
  # is not present.
  AC_CHECK_FUNC(getutent,
                 AC_DEFINE(HAVE_GETUTENT_CALLS,,
         [Define if your system has the three ???utent functions]),
                 [if test "$ac_cv_header_utmp_h" = "yes"; then
		   AC_LIBOBJ([utmp])
		 fi])
  if test "$ac_cv_header_utmp_h" = "yes"; then
    AC_CHECK_MEMBERS([struct utmp.ut_user, struct utmp.ut_name],,,
                     [#include <sys/types.h>
#include <utmp.h>])
  fi
])
