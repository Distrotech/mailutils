dnl Configuration for GNU Mailutils -- a suite of utilities for electronic mail
dnl
dnl Copyright (C) 1999,2000,2001,2002,2003,2004 Free Software Foundation, Inc.
dnl 
dnl GNU Mailutils is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl
dnl GNU Mailutils is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with GNU Mailutils; if not, write to the Free Software
dnl Foundation, Inc. 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

AC_INIT([GNU Mailutils], [0.4.1], [bug-mailutils@gnu.org], [mailutils])
AC_CONFIG_SRCDIR([mailbox/mailbox.c])
AC_CONFIG_AUX_DIR([scripts])
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

AC_PREREQ(2.59)

dnl Check for programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_PROG_YACC
AM_PROG_LEX

dnl Internationalization macros.
AM_GNU_GETTEXT(no-libtool, need-ngettext)
AM_GNU_GETTEXT_VERSION(0.13)
AC_SUBST(INTLINCS)
if test ".$BUILD_INCLUDED_LIBINTL" = ".yes" ; then
        AC_DEFINE(WITH_INCLUDED_LIBINTL,1,
                  [Define if the included libintl is used])
	INTLINCS='-I${top_builddir}/intl -I${top_srcdir}/intl'
fi

dnl Check for libraries
AC_CHECK_LIB(resolv, res_query)
LTDL_LIB=
AC_SUBST(LTDL_LIB)
if test "$enable_shared" = yes; then
  AC_CHECK_LIB(ltdl, lt_dlinit,[
    AC_DEFINE(HAVE_LIBLTDL,
	      1,
	      [Define to 1 if you have the `ltdl' library (-lltdl).])
    LTDL_LIB=-lltdl],
    [LTDL_LIB=""])
fi
MU_DEBUG_MODE

# Initialize authlibs
AUTHLIBS=../auth/libmuauth.la

dnl Check for arguments
AC_ARG_ENABLE([pam],
              AC_HELP_STRING([--disable-pam],
                             [disable pam]),
              [
case "${enableval}" in
  yes) testpam=yes ;;
  no)  testpam=no ;;
  *)   AC_MSG_ERROR([bad value ${enableval} for --disable-pam]) ;;
esac],[testpam=yes])

AC_ARG_ENABLE([pthread],
              AC_HELP_STRING([--disable-pthread],
                             [disable pthread]),
              [
case "${enableval}" in
  yes) usepthread=yes ;;
  no)  usepthread=no ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --disable-pthread) ;;
esac],[usepthread=yes])

AC_ARG_WITH([readline],
            AC_HELP_STRING([--without-readline],
                           [do not use readline]),
            [
case "${withval}" in
  yes) usereadline=yes ;;
  no)  usereadline=no ;;
  *)   AC_MSG_ERROR(bad value ${withval} for --without-readline) ;;
esac],[usereadline=yes])

AC_SUBST(IMAP_AUTHOBJS)
AC_SUBST(SERV_AUTHLIBS)
AC_SUBST(SERV_AUTHINCS)

AC_ARG_WITH([gssapi],
            AC_HELP_STRING([--with-gssapi],
                           [use GSSAPI authentication]),
            [WITH_GSSAPI=$withval],
            [WITH_GSSAPI=no])

if test "x$WITH_GSSAPI" != "xno"; then
  MU_CHECK_GSSAPI($WITH_GSSAPI)
  if test "$GSSAPI_IMPL" = "none"; then
    AC_CACHE_SAVE
    AC_MSG_RESULT([GSSAPI libraries not found])
  else
    SERV_AUTHLIBS="$SERV_AUTHLIBS $GSSAPI_LIBS"
    SERV_AUTHINCS="$SERV_AUTHINCS $GSSAPI_CFLAGS"
    IMAP_AUTHOBJS="$IMAP_AUTHOBJS auth_gss.o"
    AC_DEFINE(WITH_GSSAPI,1,[Define if mailutils is using GSSAPI])
  fi
fi

MU_CHECK_GSASL(0.0.2, [
    AC_DEFINE(WITH_GSASL,1,[Define if mailutils is using GNU SASL])
    AUTHLIBS="$AUTHLIBS $GSASL_LIBS"
    IMAP_AUTHOBJS="$IMAP_AUTHOBJS auth_gsasl.o"])

AC_SUBST(SITE_CRAM_MD5_PWD)
SITE_CRAM_MD5_PWD="\$(sysconfdir)/cram-md5.pwd"
AC_ARG_WITH(cram-passwd,
            AC_HELP_STRING([--with-cram-passwd=FILE],
                           [use keep passwords for CRAM-MD5 authentication method in FILE (default $SITE_CRAM_MD5_PWD)]),
            [case "${withval}" in
            /*) SITE_CRAM_MD5_PWD="${withval}";;
            ./*|../*) AC_MSG_ERROR(Bad value for --with-cram-passwd);;
            *)  SITE_CRAM_MD5_PWD="\$(sysconfdir)/${withval}";;
            esac])

MU_CHECK_TLS
if test "$WITH_GNUTLS" != "no"; then
  AUTHLIBS="$AUTHLIBS $TLS_LIBS"
  AC_DEFINE(WITH_GNUTLS,1,[Define if mailutils is using GNU TLS])
  AC_DEFINE(WITH_TLS,1,[Define if mailutils is using TLS])
fi

AC_ARG_WITH([guile],
            AC_HELP_STRING([--without-guile],
                           [do not build guile interface]),
            [
case "${withval}" in
  yes) useguile=yes ;;
  no)  useguile=no ;;
  *)   AC_MSG_ERROR(bad value ${withval} for --without-guile) ;;
esac],[useguile=yes])

AC_SUBST(SITE_MAIL_RC)
AC_ARG_WITH([mail-rc],
            AC_HELP_STRING([--with-mail-rc=FILE],
                           [use FILE instead of $sysconfdir/mail.rc]),
            [
case "${withval}" in
  /*) SITE_MAIL_RC="${withval}";;
  *)  SITE_MAIL_RC="\$(sysconfdir)/${withval}";;
  esac],
 [SITE_MAIL_RC="\$(sysconfdir)/mail.rc"])

MU_CONF_MAILDIR=
AC_ARG_WITH([mail-spool],
            AC_HELP_STRING([--with-mail-spool=PATH],
                           [use PATH instead of /var/spool/mail]),
            [
case "$withval" in
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
AC_ARG_WITH([gdbm],
            AC_HELP_STRING([--with-gdbm],
                           [use GNU DBM]),
            [
case "${withval}" in
  yes) use_dbm=GDBM ;;
  no)  use_dbm=no ;;
  *)   AC_MSG_ERROR(bad value ${withval} for --with-gdbm) ;;
esac])

AC_ARG_WITH([db2],
            AC_HELP_STRING([--with-db2],
                           [use Berkeley DB]),
            [
case "${withval}" in
  yes) use_dbm=BDB2 ;;
  no)  use_dbm=no ;;
  *)   AC_MSG_ERROR(bad value ${withval} for --with-db) ;;
esac])

AC_ARG_WITH([ndbm],
            AC_HELP_STRING([--with-ndbm],
                           [use NDBM]),
            [
case "${withval}" in
  yes) use_dbm=NDBM ;;
  no)  use_dbm=no ;;
  *)   AC_MSG_ERROR(bad value ${withval} for --with-ndbm) ;;
esac])

AC_ARG_WITH([dbm],
            AC_HELP_STRING([--with-dbm],
                           [use old DBM]),
            [
case "${withval}" in
  yes) use_dbm=ODBM ;;
  no)  use_dbm=no ;;
  *)   AC_MSG_ERROR(bad value ${withval} for --with-old-dbm) ;;
esac])

AC_MSG_CHECKING(for log facility)
log_facility="LOG_MAIL"
AC_ARG_WITH([log-facility],
            AC_HELP_STRING([--with-log-facility=facility],
                           [enable logging to the given facility]),
            [AC_TRY_COMPILE([#include <syslog.h>], int lf = $withval,
	     log_facility=$withval)])
AC_DEFINE_UNQUOTED(LOG_FACILITY, $log_facility,
                   [Default syslog facility to use])
AC_MSG_RESULT($log_facility)

dnl Check for headers
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_CHECK_HEADERS(errno.h fcntl.h inttypes.h libgen.h limits.h\
 malloc.h obstack.h paths.h shadow.h socket.h sys/socket.h stdarg.h stdio.h\
 stdlib.h string.h strings.h sys/file.h sysexits.h syslog.h termcap.h\
 termios.h termio.h sgtty.h utmp.h utmpx.h unistd.h)
 
dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_TYPE(ssize_t, , AC_DEFINE(ssize_t, int, [Define to int if <sys/types.h> does not define]))
AC_TYPE_SIGNAL
AC_DECL_SYS_SIGLIST
AC_STRUCT_TIMEZONE
AC_CHECK_TYPE(ino_t, , AC_DEFINE(ino_t, unsigned long, [Define to unsigned long if <sys/types.h> does not define]))
AC_CHECK_TYPE(dev_t, , AC_DEFINE(dev_t, unsigned long, [Define to unsigned long if <sys/types.h> does not define]))
AC_CHECK_TYPE(socklen_t, , AC_DEFINE(socklen_t, int, [Define to int if <sys/types.h> does not define]),
[
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SOCKET_H
# include <socket.h>
#endif
])
AC_CHECK_TYPE(long long, , AC_DEFINE(LONG_LONG, long, [Define to long if compiler does not support]))
AC_CHECK_TYPE(long double, , AC_DEFINE(LONG_DOUBLE, double, [Define to double if compiler does not support]))

AC_STRUCT_TM
AC_CHECK_MEMBERS([struct tm.tm_zone, struct tm.tm_gmtoff, struct tm.tm_isdst],
                 ,,
                 [#include <sys/types.h>
#include <$ac_cv_struct_tm>])

dnl Check for working functions

AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_ALLOCA

jm_INCLUDED_REGEX(mailbox/regex.c)

# Not use.
#AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_FNMATCH

AC_CHECK_DECLS([strtok_r, strchrnul, strndup, asprintf, vasprintf, strsignal],
 , , [
#include <stdio.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif])

AC_CHECK_DECLS(environ, , ,
  [ #include <unistd.h>
    #include <stdlib.h> ])

AH_BOTTOM([
#if !HAVE_DECL_STRTOK_R
extern char *strtok_r (char *s, const char *delim, char **save_ptr);
#endif
#if !HAVE_DECL_STRSIGNAL
extern char *strsignal (int);
#endif
])

AC_REPLACE_FUNCS(getpass setenv snprintf strncasecmp \
 strcasecmp strsignal vasprintf vsyslog)
AC_CHECK_FUNCS(mkstemp sigaction sysconf getdelim setreuid \
 setresuid seteuid vfork _exit)

AC_FUNC_SETVBUF_REVERSED
AH_BOTTOM([
#if defined (SETVBUF_REVERSED)
# define SETVBUF(str,buf,mode,size) setvbuf(str,mode,buf,size)
#else
# define SETVBUF(str,buf,mode,size) setvbuf(str,buf,mode,size)
#endif
])

MU_CHECK_FUNCS(fgetpwent strtok_r getline strndup strnlen strchrnul)

## Check for presence of utmp.h and utmp-related functions
MU_CHECK_UTMP

MU_REPLACE_GNU_GETOPT

dnl Check for libraries
USE_INCLUDED_ARGP=
AC_ARG_WITH([included-argp],
            AC_HELP_STRING([--with-included-argp],
                           [Use included argp library]),
            [USE_INCLUDED_ARGP=${withval}],
            [AC_CHECK_HEADER(argp.h,
                             [AC_CHECK_FUNCS(argp_parse, :,
                                             [USE_INCLUDED_ARGP=yes])],
                             [USE_INCLUDED_ARGP=yes])])

if test "$USE_INCLUDED_ARGP" = yes; then
       MU_LIBOBJ(argp-ba)
       MU_LIBOBJ(argp-eexst)
       MU_LIBOBJ(argp-fmtstream)
       MU_LIBOBJ(argp-fs-xinl)
       MU_LIBOBJ(argp-help)
       MU_LIBOBJ(argp-parse)
       MU_LIBOBJ(argp-pv)
       MU_LIBOBJ(argp-pvh)
       MU_LIBOBJ(argp-xinl)
       MU_LIBOBJ(pin)
       MU_HEADER(argp.h)
fi

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


dnl check if SQL support is required
AC_SUBST(SQLLIBS)
AC_ARG_WITH([mysql],
            AC_HELP_STRING([--with-mysql],
                           [enable mysql support (default no)]),
            [use_mysql=${withval}],,)

AC_ARG_WITH([postgres],
            AC_HELP_STRING([--with-postgres],
                           [enable postgres support (default no)]),
            [use_postgres=${withval}],,)

AH_TEMPLATE(USE_SQL,[Enables SQL support])
AH_TEMPLATE(HAVE_MYSQL,[Define if you have libmysql])
AH_TEMPLATE(HAVE_PGSQL,[Define if you have libpq]) 

if test x"$use_mysql" = x"yes" -a x"$use_postgres" = x"yes"; then
 AC_MSG_ERROR(Use either --with-mysql or --with-postgres, not both)
else
 if test x"$use_mysql" = x"yes"; then
   AC_CHECK_HEADER(mysql/mysql.h,
                        [MU_CHECK_LIB(mysqlclient, mysql_real_connect, "-lm",
                            [ USE_SQL=1
                              AC_DEFINE(USE_SQL)
                              AC_DEFINE(HAVE_MYSQL)
                              SQLLIBS="$mu_cv_lib_mysqlclient"
                            ],
			    [ AC_MSG_ERROR(-lmysqlclient not found) ],
                         [/usr/local/lib/mysql /usr/lib/mysql])])
 fi
 if test x"$use_postgres" = x"yes"; then
   MU_CHECK_LIB(pq, PQconnectStart, [],
		[ USE_SQL=1
                  AC_DEFINE(HAVE_PGSQL)
                  SQLLIBS=$mu_cv_lib_pq],
                [ AC_MSG_ERROR(-lpq not found) ],
                [/usr/local/pgsql/lib /usr/pgsql/lib])
 fi
fi

if test x"$USE_SQL" = x1; then
  AUTHLIBS="$AUTHLIBS $SQLLIBS"
  AC_DEFINE(USE_SQL,1,[Define this to enable SQL support])
fi

dnl Virtual domain support
MU_ENABLE_SUPPORT(virtual-domains)
AC_SUBST(SITE_VIRTUAL_PWDDIR)
AC_ARG_WITH([virtual-pwddir],
             AC_HELP_STRING([--with-virtual-pwddir=DIR],
                              [use DIR instead of $sysconfdir/domain]),[
             case "${withval}" in
		/*) SITE_VIRTUAL_PWDDIR="${withval}";;
		*)  SITE_VIRTUAL_PWDDIR="\$(sysconfdir)/${withval}";;
	     esac],
             [SITE_VIRTUAL_PWDDIR="\$(sysconfdir)/domain"])

MU_ENABLE_SUPPORT(imap)
MU_ENABLE_SUPPORT(pop)

AC_SUBST(BUILD_SMTP_PROGRAMS)
AC_SUBST(RUN_SMTP_DEJATOOL)
MU_ENABLE_SUPPORT(smtp, [BUILD_SMTP_PROGRAMS='$(PROGRAMS_SMTP)'
RUN_SMTP_DEJATOOL='$(SMTP_DEJATOOL)'])
MU_ENABLE_SUPPORT(sendmail)

AC_SUBST(BUILD_MH_PROGRAMS)
AC_SUBST(BUILD_MH_LIBRARIES)
AC_SUBST(BUILD_MH_EXEC_HOOK)

MU_ENABLE_SUPPORT(mh,
                  [BUILD_MH_PROGRAMS='$(PROGRAMS_MH)';
                   BUILD_MH_LIBRARIES='$(LIBRARIES_MH)';
                   BUILD_MH_EXEC_HOOK=mh_finish_install])
MU_ENABLE_SUPPORT(maildir)

dnl AC_ARG_ENABLE([experimental],
dnl               AC_HELP_STRING([--enable-experimental],
dnl                              [build experimental and/or unfinished utilities]),
dnl               [MU_BUILD_EXPERIMENTAL="${enableval}"],
dnl               [MU_BUILD_EXPERIMENTAL=no])

AC_ARG_ENABLE([mh-utils],
              AC_HELP_STRING([--enable-mh-utils],
                             [Build MH utilities]),
              [MU_BUILD_MH="${enableval}"],
              [MU_BUILD_MH=no])

AC_SUBST(MH_BIN_DIR)
if test ".$MU_BUILD_MH" = ".no"; then
	BUILD_MH_PROGRAMS=
	BUILD_MH_LIBRARIES=
	BUILD_MH_EXEC_HOOK=
fi

AC_ARG_WITH([mh-bindir],
	    AC_HELP_STRING([--with-mh-bindir],
                           [Set the directory where to install the MH binaries. If it starts with '/' it is taken as an absolute path specification, otherwise ${prefix} is prepended to it. (Default: ${exec_prefix}/bin/mu-mh)]),
		    [case "${withval}" in
		     /*)  MH_BIN_DIR=${withval};;
                     *)   MH_BIN_DIR="\${prefix}/${withval}";;
                     esac],
                    [MH_BIN_DIR='${exec_prefix}/bin/mu-mh'])

if test x"$testpam" = x"yes"; then
	AC_CHECK_HEADERS(security/pam_appl.h)
	if test x"$ac_cv_header_security_pam_appl_h" = x"yes"; then
		AC_CHECK_LIB(dl, dlopen, [LDL=-ldl], [LDL=])
		AC_CHECK_LIB(pam, pam_start,
			[AUTHLIBS="$AUTHLIBS -lpam $LDL"
                         ac_cv_have_libpam="yes"],, $LDL)
		AC_DEFINE(USE_LIBPAM,1,[Enable use of PAM])
	fi
fi

AC_CHECK_HEADERS(crypt.h)
AC_CHECK_DECLS(crypt, , ,[
#include <unistd.h>
#ifdef HAVE_CRYPT_H
# include <crypt.h>
#endif])
AH_BOTTOM([
#if !HAVE_DECL_CRYPT
char *crypt(const char *key, const char *salt);
#endif])

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
for lib in ncurses curses termcap
do
	AC_CHECK_LIB($lib, tputs, [CURSES_LIBS="-l$lib"; break])
done

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
                     MU_DB2_CURSOR
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
AC_SUBST(GUILE_BINDIR)
AC_SUBST(GUILE_SNARF_VERSION)
AC_SUBST(GUIMB)
AC_SUBST(MU_SCM)
AC_SUBST(LIBMU_SCM)
AC_SUBST(LIBMU_SCM_DEPS)
AC_SUBST(GUILE_PKGDATA)
AC_SUBST(GUILE_BUILT_SOURCES)

if test x"$useguile" = x"yes"; then
	MU_CHECK_GUILE(,[
            AC_DEFINE(WITH_GUILE,1,[Enable Guile support])
            GUILE_INCLUDES=`guile-config compile`
  	    GUILE_LIBS=`guile-config link`
	    GUILE_BINDIR=`guile-config info bindir`
	    GUIMB=guimb
            MU_SCM=libmu_scm.la
            LIBMU_SCM=../libmu_scm/libmu_scm.la
	    LIBMU_SCM_DEPS="../mailbox/mbox/libmu_mbox.la ../mailbox/imap/libmu_imap.la ../mailbox/pop/libmu_pop.la ../mailbox/mh/libmu_mh.la"
            GUILE_PKGDATA="guile-procedures.txt mailutils.scm"
	    GUILE_BUILT_SOURCES='$(DOT_X_FILES) $(DOT_DOC_FILES)'
            case "$GUILE_VERSION" in
	      14) GUILE_SNARF_VERSION="1.4";;
	      16|17) GUILE_SNARF_VERSION="1.6";;
	      *) GUILE_SNARF_VERSION="1.6";;
            esac
         ],[useguile=no])
fi

dnl Check for Emacs site-lisp directory
AM_PATH_LISPDIR

if test "$EMACS" != "no"; then
	lisp_LISP='$(LISPSRC)'
fi
AC_SUBST(lisp_LISP)

dnl Define ARG_UNUSED macro
dnl
AH_BOTTOM([
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __attribute__(x)
#endif

#ifndef ARG_UNUSED
# define ARG_UNUSED __attribute__ ((__unused__))
#endif
])

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

S=`echo "$MU_LIBOBJS" |
                  sed 's,\.[[^.]]* ,$U&,g;s,\.[[^.]]*$,$U&,'`
MU_LTLIBOBJS=`echo "$S" |
                sed 's,\.[[^.]]* ,.lo ,g;s,\.[[^.]]*$,.lo,'`
AC_SUBST(MU_LTLIBOBJS)

AC_SUBST(SIEVE_MODDIR)
SIEVE_MODDIR='$(libdir)/$(PACKAGE)'

dnl Output Makefiles
AC_CONFIG_FILES([Makefile
 auth/Makefile
 comsat/Makefile
 doc/Makefile
 doc/man/Makefile
 doc/rfc/Makefile
 doc/texinfo/Makefile
 dotlock/Makefile
 examples/Makefile
 frm/Makefile
 frm/testsuite/Makefile
 from/Makefile
 from/testsuite/Makefile
 guimb/Makefile
 guimb/scm/Makefile
 imap4d/Makefile
 imap4d/testsuite/Makefile
 headers/Makefile
 headers/posix/Makefile
 include/Makefile
 include/mailutils/Makefile
 include/mailutils/gnu/Makefile
 include/mailutils/sys/Makefile
 intl/Makefile
 lib/Makefile
 libmu_scm/Makefile
 libsieve/Makefile
 libsieve/extensions/Makefile
 m4/Makefile
 mail/Makefile
 mail/testsuite/Makefile
 mail.local/Makefile
 mail.local/testsuite/Makefile
 mail.remote/Makefile
 mail.remote/testsuite/Makefile
 mailbox/Makefile
 mailbox/mbox/Makefile
 mailbox/mh/Makefile
 mailbox/maildir/Makefile
 mailbox/pop/Makefile
 mailbox/imap/Makefile
 mailbox/include/Makefile
 mailbox/testsuite/Makefile
 messages/Makefile
 messages/testsuite/Makefile
 mh/Makefile
 movemail/Makefile
 po/Makefile.in
 pop3d/Makefile
 pop3d/testsuite/Makefile
 readmsg/Makefile
 readmsg/testsuite/Makefile
 scripts/Makefile
 scripts/guile-1.4/Makefile
 scripts/guile-1.6/Makefile
 scripts/mailutils.spec
 sieve/Makefile
 sieve/testsuite/Makefile
 testsuite/Makefile
 ])
AC_OUTPUT

