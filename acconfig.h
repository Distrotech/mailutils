/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Define if using libpam */
#undef USE_LIBPAM

/* Define if have snprintf() */
#undef HAVE_SNPRINTF

/* Define if struct tm has a tm_zone member */
#undef HAVE_TM_ZONE

/* Define if struct tm has a tm_isdst member */
#undef HAVE_TM_ISDST

/* Define if struct tm has a tm_gmtoff member */
#undef HAVE_TM_GMTOFF

/* Define to proper type when ino_t is not declared */
#undef ino_t

/* Define to proper type when dev_t is not declared */
#undef dev_t

/* Define if enable Posix Thread */
#undef WITH_PTHREAD

/* Define if using GNU DBM */
#undef WITH_GDBM

/* Define if using Berkeley DB2 */
#undef WITH_BDB2

/* Define if using NDBM */
#undef WITH_NDBM

/* Define if using OLD_DBM */
#undef WITH_OLD_DBM

/* Define if using libreadline */
#undef WITH_READLINE

/* Define when using GSSAPI */
#undef WITH_GSSAPI

/* Define when using Guile */
#undef WITH_GUILE

/* Define _REENTRANT when using threads.  */
#undef _REENTRANT

/* Define the default loggin facility.  */
#undef LOG_FACILITY

/* Define HAVE_MYSQL when using mysql */
#undef HAVE_MYSQL

/* Define if virtual domain support is enabled */
#undef USE_VIRTUAL_DOMAINS

/* Define if libc has obstack functions */
#undef HAVE_OBSTACK

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef ssize_t

#undef MU_CONF_MAILDIR

#undef PROGRAM_INVOCATION_NAME_DEFINED
#undef HAVE_PROGRAM_INVOCATION_NAME

@BOTTOM@

#ifdef HAVE_PATHS_H
# include <paths.h>
#endif

#ifndef _PATH_MAILDIR
# if (defined(sun) && defined(__svr4__)) || defined(__SVR4)
#  define _PATH_MAILDIR	"/var/mail"
# else  
#  define _PATH_MAILDIR "/usr/spool/mail"
# endif
#endif

#ifdef MU_CONF_MAILDIR
# define MU_PATH_MAILDIR MU_CONF_MAILDIR
#else
# define MU_PATH_MAILDIR _PATH_MAILDIR "/"
#endif

/* Newer versions of readline have rl_completion_matches */
#ifndef HAVE_RL_COMPLETION_MATCHES
# define rl_completion_matches completion_matches
#endif

#ifndef PROGRAM_INVOCATION_NAME_DECLARED
extern char *program_invocation_short_name;
extern char *program_invocation_name;
#endif
