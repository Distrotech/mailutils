dnl Process this file with autoconf to procude a configure script.
AC_INIT([GNU Mailutils], [0.0.9d], [bug-mailutils@gnu.org])
AC_CONFIG_SRCDIR([mailbox/mailbox.c])
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AC_PREREQ(2.52)

dnl Check for programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_LIBTOOL
AC_PROG_YACC
AM_PROG_LEX

dnl Check for libraries
AC_CHECK_LIB(resolv, res_query)

MU_DEBUG_MODE

# Initialize authlibs
AUTHLIBS=../auth/libmuauth.la

dnl Check for arguments
AC_ARG_ENABLE(pam,
 [  --disable-pam           disable pam],
 [case "${enableval}" in
  yes) testpam=yes ;;
  no)  testpam=no ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --disable-pam) ;;
 esac],[testpam=yes])

AC_ARG_ENABLE(pthread,
 [  --disable-pthread       disable pthread],
 [case "${enableval}" in
  yes) usepthread=yes ;;
  no)  usepthread=no ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --disable-pthread) ;;
  esac],[usepthread=yes])

AC_ARG_WITH(readline,
 [  --without-readline      do not use readline],
 [case "${withval}" in
  yes) usereadline=yes ;;
  no)  usereadline=no ;;
  *)   AC_MSG_ERROR(bad value ${withval} for --without-readline) ;;
  esac],[usereadline=yes])

AC_ARG_WITH(gssapi,
 [  --with-gssapi           use GSSAPI authentication ],
 [WITH_GSSAPI=$withval],
 [WITH_GSSAPI=no])

if test "x$WITH_GSSAPI" != "xno"; then
  MU_CHECK_GSSAPI($WITH_GSSAPI)
  if test "$GSSAPI_IMPL" = "none"; then
    AC_CACHE_SAVE
    AC_MSG_RESULT([GSSAPI libraries not found])
  else
    AUTHLIBS="$AUTHLIBS $GSSAPI_LIBS"
    AUTHINCS="$AUTHINCS $GSSAPI_CFLAGS"
    AUTHOBJS="$AUTHOBJS auth_gss.o"
    AC_DEFINE(WITH_GSSAPI,1,[Define if mailutils is using GSSAPI])
  fi
fi
AC_SUBST(AUTHOBJS)

AC_ARG_WITH(guile,
 [  --without-guile         do not build guile interface],
 [case "${withval}" in
  yes) useguile=yes ;;
  no)  useguile=no ;;
  *)   AC_MSG_ERROR(bad value ${withval} for --without-guile) ;;
  esac],[useguile=yes])

AC_SUBST(SITE_MAIL_RC)
AC_ARG_WITH(mail-rc,
 [  --with-mail-rc=FILE     use FILE instead of \$sysconfdir/mail.rc],
 [case "${withval}" in
  /*) SITE_MAIL_RC="${withval}";;
  *)  SITE_MAIL_RC="\$(sysconfdir)/${withval}";;
  esac],
 [SITE_MAIL_RC="\$(sysconfdir)/mail.rc"])

MU_CONF_MAILDIR=
AC_ARG_WITH(mail-spool,
 [  --with-mail-spool=PATH  use PATH instead of /var/spool/mail],
 [case "$withval" in
  /*/) MU_CONF_MAILDIR="\"${withval}\"";;
  /*)  MU_CONF_MAILDIR="\"${withval}/\"";;
  *:*[[/=]]) MU_CONF_MAILDIR="\"${withval}\"";;
  *)  AC_MSG_ERROR(--with-mail-spool must be an absolute path or a valid URL);;
  esac])

if test "x$MU_CONF_MAILDIR" != x; then
  AC_DEFINE_UNQUOTED(MU_CONF_MAILDIR,$MU_CONF_MAILDIR,
                     [Pathname of the configuration directory])
fi
AH_BOTTOM([
#ifdef HAVE_PATHS_H
# include <paths.h>
#endif
#ifndef _PATH_MAILDIR
# if (defined(sun) && defined(__svr4__)) || defined(__SVR4)
#  define _PATH_MAILDIR "/var/mail"
# else
#  define _PATH_MAILDIR "/usr/spool/mail"
# endif
#endif
#ifdef MU_CONF_MAILDIR
# define MU_PATH_MAILDIR MU_CONF_MAILDIR
#else
# define MU_PATH_MAILDIR _PATH_MAILDIR "/"
#endif])

use_dbm=no
AC_ARG_WITH(gdbm,
 [  --with-gdbm             use GDBM],
 [case "${withval}" in
  yes) use_dbm=GDBM ;;
  no)  use_dbm=no ;;
  *)   AC_MSG_ERROR(bad value ${withval} for --with-gdbm) ;;
  esac])

AC_ARG_WITH(db2,
 [  --with-db2              use Berkeley DB],
 [case "${withval}" in
  yes) use_dbm=BDB2 ;;
  no)  use_dbm=no ;;
  *)   AC_MSG_ERROR(bad value ${withval} for --with-db) ;;
  esac])

AC_ARG_WITH(ndbm,
 [  --with-ndbm             use NDBM],
 [case "${withval}" in
  yes) use_dbm=NDBM ;;
  no)  use_dbm=no ;;
  *)   AC_MSG_ERROR(bad value ${withval} for --with-ndbm) ;;
  esac])

AC_ARG_WITH(dbm,
 [  --with-dbm              use old DBM],
 [case "${withval}" in
  yes) use_dbm=ODBM ;;
  no)  use_dbm=no ;;
  *)   AC_MSG_ERROR(bad value ${withval} for --with-old-dbm) ;;
  esac])

AC_MSG_CHECKING(for log facility)
log_facility="LOG_MAIL"
AC_ARG_WITH(log-facility,
 [  --with-log-facility=facility   enable logging to the given facility],
 AC_TRY_COMPILE([#include <syslog.h>], int lf = $withval,
	log_facility=$withval))
AC_DEFINE_UNQUOTED(LOG_FACILITY, $log_facility,
                   [Default syslog facility to use])
AC_MSG_RESULT($log_facility)

dnl Check for headers
AC_HEADER_STDC
AC_HEADER_DIRENT
AM_C_PROTOTYPES
AC_CHECK_HEADERS(argp.h errno.h fcntl.h inttypes.h libgen.h limits.h malloc.h \
 paths.h shadow.h stdarg.h stdio.h stdlib.h string.h strings.h sys/file.h \
 sysexits.h syslog.h termios.h termio.h sgtty.h utmp.h utmpx.h unistd.h obstack.h \
 termcap.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_TYPE(ssize_t, int)
AC_TYPE_SIGNAL
AC_DECL_SYS_SIGLIST
AC_STRUCT_TIMEZONE
AC_CHECK_TYPE(ino_t, unsigned long)
AC_CHECK_TYPE(dev_t, unsigned long)

AC_STRUCT_TM
AC_CHECK_MEMBERS([struct tm.tm_zone, struct tm.tm_gmtoff, struct tm.tm_isdst],
                 ,,
                 [#include <sys/types.h>
#include <$ac_cv_struct_tm>])

dnl Check for working functions

jm_FUNC_MALLOC
jm_FUNC_REALLOC
AC_FUNC_ALLOCA

jm_INCLUDED_REGEX(lib/regex.c)

# Not use.
#AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_FNMATCH

AC_CHECK_DECLS([strtok_r, strchrnul, strndup, asprintf, vasprintf] , , , [
#include <stdio.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif])

AH_BOTTOM([
#if !HAVE_DECL_STRTOK_R
extern char *strtok_r (char *s, const char *delim, char **save_ptr);
#endif])

AC_REPLACE_FUNCS(fgetpwent getpass setenv snprintf strtok_r strncasecmp \
 strcasecmp strsignal vasprintf strndup strnlen strchrnul)
AC_CHECK_FUNCS(mkstemp sigaction sysconf getdelim vsyslog setreuid \
 setresuid seteuid vfork _exit)

## Check for presence of utmp.h and utmp-related functions
MU_CHECK_UTMP

dnl Check for libraries
AC_CHECK_FUNCS(argp_parse, :,
               [
	       AC_LIBOBJ(argp-ba)
	       AC_LIBOBJ(argp-eexst)
	       AC_LIBOBJ(argp-fmtstream)
	       AC_LIBOBJ(argp-fs-xinl)
	       AC_LIBOBJ(argp-help)
	       AC_LIBOBJ(argp-parse)
	       AC_LIBOBJ(argp-pv)
	       AC_LIBOBJ(argp-pvh)
	       AC_LIBOBJ(argp-xinl)
	       ])

AC_MSG_CHECKING(whether extern program_invocation_name is present)
AC_TRY_COMPILE([#include <argp.h>
#include <errno.h>],
[ program_invocation_name = "test"; ],
[AC_DEFINE(PROGRAM_INVOCATION_NAME_DECLARED,1,
           [Define if program_invocation_name is declared in argp.h])
 AC_MSG_RESULT(yes)],
[AC_MSG_RESULT(no)])

AH_BOTTOM([
#ifndef PROGRAM_INVOCATION_NAME_DECLARED
extern char *program_invocation_short_name;
extern char *program_invocation_name;
#endif])


AC_MSG_CHECKING(whether program_invocation_name is declared)
AC_TRY_COMPILE([#include <argp.h>
#include <errno.h>],
[
#ifndef PROGRAM_INVOCATION_NAME_DECLARED
	extern char *program_invocation_name;
#endif
	program_invocation_name = "test";
],
[AC_DEFINE(HAVE_PROGRAM_INVOCATION_NAME,1,
           [Define if the variable program_invocation_name exists])
 AC_MSG_RESULT(yes)],
[AC_MSG_RESULT(no)])

AC_CHECK_FUNCS(_obstack_begin,
               [AC_DEFINE(HAVE_OBSTACK,1,[Define if GNU obstack is present])],
               [AC_LIBOBJ(obstack)])


dnl check if mysql support was added
AC_SUBST(BUILD_LIBMYSQL)
AC_SUBST(MYSQLLIBS)
AC_ARG_ENABLE(mysql, [  --enable-mysql  enable mysql support (default no)], [use_mysql="yes"],,)
if test x"$use_mysql" = x"yes"; then
	AC_CHECK_HEADER(mysql/mysql.h,
	[AUTHLIBS="$AUTHLIBS -lmysqlclient -lm -L/usr/lib/mysql -L/usr/local/lib/mysql"
	AC_DEFINE(HAVE_MYSQL,1,[Define this if you have mysql libraries])])
fi

dnl Virtual domain support, at least for pop3d
AC_ARG_ENABLE(virtual-domains, [  --enable-virtual-domains  enable virtual domain support (default no)], [use_virtual_domains="yes"],,)
if test x"$use_virtual_domains" = x"yes"; then
   AC_DEFINE(USE_VIRTUAL_DOMAINS,1,[Enable use of virtual domains])
   AC_SUBST(SITE_VIRTUAL_PWDDIR)
   AC_ARG_WITH(virtual-pwddir,
    [  --with-virtual-pwddir=DIR   use DIR instead of \$sysconfdir/domain],
    [case "${withval}" in
     /*) SITE_VIRTUAL_PWDDIR="${withval}";;
     *)  SITE_VIRTUAL_PWDDIR="\$(sysconfdir)/${withval}";;
    esac],
    [SITE_VIRTUAL_PWDDIR="\$(sysconfdir)/domain"])
fi


if test x"$testpam" = x"yes"; then
	AC_CHECK_HEADERS(security/pam_appl.h)
	if test x"$ac_cv_header_security_pam_appl_h" = x"yes"; then
		AC_CHECK_LIB(dl, dlopen, [LDL=-ldl], [LDL=])
		AC_CHECK_LIB(pam, pam_start,
			AUTHLIBS="$AUTHLIBS -lpam $LDL" ac_cv_have_libpam="yes",, $LDL)
		AC_DEFINE(USE_LIBPAM,1,[Enable use of PAM])
	fi
fi

AC_CHECK_HEADERS(crypt.h)
AC_CHECK_LIB(crypt, crypt)

AC_SUBST(AUTHLIBS)
AC_SUBST(AUTHINCS)

#When using thread support some platforms need -D_REENTRANT to get the
#right prototypes including errno.
dnl Check threading support
# We have to rearrange things a little, it appears that the new autoconf
# does not like long cascading AC_CHECK_LIB.
if test x"$usepthread" = x"yes"; then
	AC_CHECK_LIB(pthread, pthread_cancel, have_libpthread=yes,
		 have_libpthread=no)
        if test x"$have_libpthread" = x"yes"; then
		AC_CHECK_HEADERS(pthread.h,
                                 AC_DEFINE(WITH_PTHREAD,1,
                                           [Define this to enable POSIX threads safety]))
			LIBS="$LIBS -lpthread"
			AC_DEFINE(_REENTRANT,1,[Reentrant calls])
			AC_CHECK_FUNCS(pthread_rwlock_init)
	else
		AC_SEARCH_LIBS(pthread_cancel, pthread,
			[AC_CHECK_FUNCS(pthread_rwlock_init)
				AC_CHECK_HEADERS(pthread.h,
					AC_DEFINE(WITH_PTHREAD,1,
                                                  [Define this to enable POSIX threads safety]))
					AC_DEFINE(_REENTRANT,1,[Reentrant calls])])
	fi
fi

dnl Do we need extra libs for networking?
AC_CHECK_FUNC(gethostbyname, [true],
	AC_CHECK_LIB(nsl, gethostbyaddr, LIBS="-lnsl $LIBS"))

AC_CHECK_FUNC(socket, [true],
	AC_CHECK_LIB(socket, socket, LIBS="-lsocket $LIBS"))

dnl Check for Curses libs.
AC_CHECK_LIB(ncurses, tputs, CURSES_LIBS="-lncurses",
	AC_CHECK_LIB(curses, tputs, CURSES_LIBS="-lcurses"))

AC_SUBST(CURSES_LIBS)

dnl Check for GNU Readline
AC_SUBST(READLINE_LIBS)

if test x"$usereadline" = x"yes"; then

	dnl FIXME This should only link in the curses libraries if it's
	dnl really needed!
	
	saved_LIBS=$LIBS
	LIBS="$LIBS $CURSES_LIBS"
	AC_CHECK_LIB(readline, readline, mu_have_readline=yes)
	LIBS=$saved_LIBS
	
	if test x"$mu_have_readline" = x"yes"; then
		AC_CHECK_HEADERS(readline/readline.h,
			AC_DEFINE(WITH_READLINE,1,[Enable use of readline]))
			READLINE_LIBS="-lreadline $CURSES_LIBS"
			saved_LIBS=$LIBS
			LIBS="$LIBS $READLINE_LIBS"
			AC_CHECK_FUNCS(rl_completion_matches)
			LIBS=$saved_LIBS
	else
		AC_MSG_WARN(readline requested but does not seem to be installed)
	fi

fi
AH_BOTTOM([
/* Newer versions of readline have rl_completion_matches */
#ifndef HAVE_RL_COMPLETION_MATCHES
# define rl_completion_matches completion_matches
#endif])

dnl Check for DBM

AC_SUBST(POPAUTH)
case "$use_dbm" in
GDBM)
	AC_CHECK_LIB(gdbm, gdbm_open,
                     [AC_CHECK_HEADERS(gdbm.h,
                                       AC_DEFINE(WITH_GDBM,1,
                                                 [Enable use of GNU DBM library]))
                     LIBS="$LIBS -lgdbm"
                     POPAUTH=popauth]);;
BDB2)
	AC_CHECK_LIB(db, db_open,
                     [AC_CHECK_HEADERS(db.h,
                                       AC_DEFINE(WITH_BDB2,1,
                                                 [Enable use of Berkeley DB]))
                     LIBS="$LIBS -ldb"
		     POPAUTH=popauth]);;
NDBM)
	AC_CHECK_LIB(ndbm, dbm_open,
                     [AC_CHECK_HEADERS(ndbm.h,
                                       AC_DEFINE(WITH_NDBM,1,
                                                 [Enable use of NDBM]))
                     LIBS="$LIBS -lndbm"
		     POPAUTH=popauth]);;
ODBM)
	AC_CHECK_LIB(dbm, dbminit,
                     [AC_CHECK_HEADERS(dbm.h,
                                       AC_DEFINE(WITH_OLD_DBM,1,
                                                 [Enable use of old DBM library]))
                     LIBS="$LIBS -ldbm"
		     POPAUTH=popauth]);;
esac

dnl Check for Guile
AC_SUBST(GUILE_INCLUDES)
AC_SUBST(GUILE_LIBS)
AC_SUBST(GUIMB)
AC_SUBST(MU_SCM)
AC_SUBST(LIBMU_SCM)
AC_SUBST(GUILE_PROCEDURES_TXT)
AC_SUBST(GUILE_BUILT_SOURCES)
if test x"$useguile" = x"yes"; then
	AC_PATH_PROG(GUILE_CONFIG, guile-config, no, $PATH)
	AC_CHECK_HEADER(libguile.h, :, [GUILE_CONFIG=no])
	if test x$GUILE_CONFIG != xno; then
		AC_DEFINE(WITH_GUILE,1,[Enable Guile support])
  		GUILE_INCLUDES=`guile-config compile`
  		GUILE_LIBS=`guile-config link`
		GUIMB=guimb
		MU_SCM=libmu_scm.a
		LIBMU_SCM=../libmu_scm/libmu_scm.a
		GUILE_PROCEDURES_TXT=guile-procedures.txt
		GUILE_BUILT_SOURCES='$(DOT_X_FILES) $(DOT_DOC_FILES)'
	fi
fi

dnl Make sysconfdir available to the application
dnl This must be done LAST, since CPPFLAGS is passed by configure
dnl to build tests, but this relies on being run in make to
dnl get sysconfdir expanded.

CPPFLAGS="$CPPFLAGS -DSYSCONFDIR=\\\"\$(sysconfdir)\\\""

# This is necessary so that .o files in LIBOBJS are also built via
# the ANSI2KNR-filtering rules.
LIB@&t@OBJS=`echo "$LIB@&t@OBJS" |
                  sed 's,\.[[^.]]* ,$U&,g;s,\.[[^.]]*$,$U&,'`
LTLIBOBJS=`echo "$LIB@&t@OBJS" |
                sed 's,\.[[^.]]* ,.lo ,g;s,\.[[^.]]*$,.lo,'`
AC_SUBST(LTLIBOBJS)

dnl Output Makefiles
AC_CONFIG_FILES([Makefile mailutils.spec
 include/Makefile include/mailutils/Makefile
 m4/Makefile
 doc/Makefile doc/man/Makefile doc/rfc/Makefile doc/texinfo/Makefile
 lib/Makefile lib/posix/Makefile
 testsuite/Makefile
 mailbox/Makefile
 auth/Makefile
 mailbox/include/Makefile
 imap4d/Makefile
 imap4d/testsuite/Makefile
 from/Makefile
 mail/Makefile
 mail/testsuite/Makefile
 pop3d/Makefile
 pop3d/testsuite/Makefile
 frm/Makefile
 sieve/Makefile
 messages/Makefile
 scripts/Makefile
 libmu_scm/Makefile
 guimb/Makefile
 guimb/scm/Makefile
 readmsg/Makefile
 comsat/Makefile
 mail.local/Makefile
 mail.remote/Makefile
 mh/Makefile
 dotlock/Makefile
 ])
AC_OUTPUT

