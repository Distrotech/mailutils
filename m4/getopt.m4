## $Id$

## Check for getopt_long. This can't be done in AC_CHECK_FUNCS since
## the function can be present in different libraries (namely, libmysqlclient)
## but the necessary header files may be absent, thus AC_CHECK_FUNCS will
## mark function as existent, whereas the compilation will bail out.

AH_TEMPLATE(HAVE_GNU_GETOPT, [Define if your system has GNU getopt functions])

AC_DEFUN([MU_REPLACE_GNU_GETOPT],
[
 AC_CHECK_HEADER([getopt.h],
		 mu_cv_have_getopt_h=yes
                 AC_DEFINE(HAVE_GETOPT_H,1,[Define if the system has getopt.h]),
                 mu_cv_have_getopt_h=no)
 AC_CACHE_CHECK([for GNU getopt], mu_cv_have_gnu_getopt,
  [
AC_TRY_RUN([
#include <unistd.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

struct option longopt[] = {
	"help",    no_argument,       0, 'h',
        (char*)0
};

main(argc, argv)
int argc; char **argv;
{
	getopt_long_only(argc, argv, "h", longopt, (int*)0);
	return 0;
}             ],
              mu_cv_have_gnu_getopt=yes,
              mu_cv_have_gnu_getopt=no,
              mu_cv_have_gnu_getopt=no)])

 if test x"$mu_cv_have_gnu_getopt" != xyes ; then
   mu_cv_have_getopt_h=no
   MU_LIBOBJ(getopt)
   MU_LIBOBJ(getopt1)
 else
   AC_DEFINE(HAVE_GNU_GETOPT)
 fi
 if test "$mu_cv_have_getopt_h" = no; then
   MU_HEADER(getopt.h)
 fi
])




