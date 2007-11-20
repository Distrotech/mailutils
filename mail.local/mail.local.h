/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005, 
   2007  Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif

#include <sys/types.h>
#include <errno.h>
#include <grp.h>
#include <netdb.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#ifdef HAVE_SYSEXITS_H
# include <sysexits.h>
#else
# define EX_OK          0       /* successful termination */
# define EX__BASE       64      /* base value for error messages */
# define EX_USAGE       64      /* command line usage error */
# define EX_DATAERR     65      /* data format error */
# define EX_NOINPUT     66      /* cannot open input */
# define EX_NOUSER      67      /* addressee unknown */
# define EX_NOHOST      68      /* host name unknown */
# define EX_UNAVAILABLE 69      /* service unavailable */
# define EX_SOFTWARE    70      /* internal software error */
# define EX_OSERR       71      /* system error (e.g., can't fork) */
# define EX_OSFILE      72      /* critical OS file missing */
# define EX_CANTCREAT   73      /* can't create (user) output file */
# define EX_IOERR       74      /* input/output error */
# define EX_TEMPFAIL    75      /* temp failure; user is invited to retry */
# define EX_PROTOCOL    76      /* remote error in protocol */
# define EX_NOPERM      77      /* permission denied */
# define EX_CONFIG      78      /* configuration error */
# define EX__MAX        78      /* maximum listed value */
#endif

#ifndef INADDR_LOOPBACK
# define INADDR_LOOPBAK 0x7f000001
#endif

#include <mailutils/attribute.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/list.h>
#include <mailutils/locker.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/mutil.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>
#include <mailutils/mu_auth.h>
#include <mailutils/libsieve.h>
#include <mailutils/nls.h>

#include <mu_dbm.h>
#include <mu_asprintf.h>

#if defined (USE_DBM) || defined (USE_SQL)
# define USE_MAILBOX_QUOTAS 1
#endif

/* Debug */
extern int debug_level;
#define dbg() if (debug_level) debug

/* mailquota settings */
#define MQUOTA_OK         0
#define MQUOTA_EXCEEDED   1
#define MQUOTA_UNLIMITED  2

extern char *quotadbname;
extern char *quota_query;
extern int exit_code;

extern void setgroupquota (char *str);
extern int check_quota (struct mu_auth_data *auth,
			mu_off_t size, mu_off_t *rest);

int mda (mu_mailbox_t mbx, char *username);
int switch_user_id (struct mu_auth_data *auth, int user);
void mailer_err (char *msg, char *arg, char *fmt, ...)
     MU_PRINTFLIKE(3,4);

#ifdef WITH_GUILE
struct mda_data
{
  mu_mailbox_t mbox;
  char *progfile;
  char *progfile_pattern;
  char **argv;
};

int prog_mda (struct mda_data *data);

extern int debug_guile;
#endif

