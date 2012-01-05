/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

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
#include <mailutils/util.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>
#include <mailutils/mu_auth.h>
#include <mailutils/sieve.h>
#include <mailutils/nls.h>
#include <mailutils/daemon.h>
#include <mailutils/acl.h>
#include <mailutils/server.h>
#include <mailutils/cctype.h>
#include <mailutils/io.h>
#include <mailutils/dbm.h>

#if defined (ENABLE_DBM) || defined (USE_SQL)
# define USE_MAILBOX_QUOTAS 1
#endif

#include "mailutils/libargp.h"

#include "tcpwrap.h"

/* Debug */
extern int debug_level;
#define dbg() if (debug_level) debug

/* mailquota settings */
#define MQUOTA_OK         0
#define MQUOTA_EXCEEDED   1
#define MQUOTA_UNLIMITED  2

#define MAXFD 64
#define EX_QUOTA() (ex_quota_tempfail ? EX_TEMPFAIL : EX_UNAVAILABLE)

enum maidag_mode
  {
    mode_mda,
    mode_url,
    mode_lmtp
  };

extern enum maidag_mode maidag_mode;
extern int exit_code;
extern int log_to_stderr;
extern int multiple_delivery;
extern int ex_quota_tempfail;
extern uid_t current_uid;
extern char *quotadbname;  
extern char *quota_query;  

extern char *forward_file;
extern int forward_file_checks;

extern char *sender_address;       
extern mu_list_t script_list;
extern char *message_id_header;
extern int sieve_debug_flags; 
extern int sieve_enable_log;  

extern mu_m_server_t server;
extern char *lmtp_url_string;
extern int reuse_lmtp_address;
extern mu_list_t lmtp_groups;
extern mu_acl_t maidag_acl;
extern int maidag_transcript;

void close_fds (void);
int switch_user_id (struct mu_auth_data *auth, int user);

typedef int (*maidag_delivery_fn) (mu_message_t, char *, char **);

int deliver_to_url (mu_message_t msg, char *dest_id, char **errp);
int deliver_to_user (mu_message_t msg, char *dest_id, char **errp);

int maidag_stdio_delivery (maidag_delivery_fn fun, int argc, char **argv);
int maidag_lmtp_server (void);
int lmtp_connection (int fd, struct sockaddr *sa, int salen,
		     struct mu_srv_config *pconf,
		     void *data);

void maidag_error (const char *fmt, ...) MU_PRINTFLIKE(1, 2);
void notify_biff (mu_mailbox_t mbox, char *name, size_t size);
void guess_retval (int ec);

int sieve_test (struct mu_auth_data *auth, mu_message_t msg);
int check_quota (struct mu_auth_data *auth, mu_off_t size, mu_off_t *rest);

enum maidag_forward_result
  {
    maidag_forward_none,
    maidag_forward_ok,
    maidag_forward_metoo,
    maidag_forward_error
  };

enum maidag_forward_result maidag_forward (mu_message_t msg,
					   struct mu_auth_data *auth,
					   char *fwfile);

/* Scripting support */
typedef int (*maidag_script_fun) (mu_message_t msg,
				  struct mu_auth_data *auth,
				  const char *prog);

extern maidag_script_fun script_handler;

struct maidag_script
{
  maidag_script_fun fun;   /* Handler function */
  const char *pat;         /* Script name pattern */
};

maidag_script_fun script_lang_handler (const char *lang);
maidag_script_fun script_suffix_handler (const char *name);
int script_register (const char *pattern);
int script_apply (mu_message_t msg, struct mu_auth_data *auth);

/* guile.c */
extern int debug_guile;
int scheme_check_msg (mu_message_t msg, struct mu_auth_data *auth,
		      const char *prog);

/* python.c */
int python_check_msg (mu_message_t msg, struct mu_auth_data *auth,
		      const char *prog);

/* sieve.c */
int sieve_check_msg (mu_message_t msg, struct mu_auth_data *auth,
		     const char *prog);

