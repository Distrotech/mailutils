/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003, 2004, 
   2005, 2006, 2007, 2008 Free Software Foundation, Inc.

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

#ifndef _IMAP4D_H
#define _IMAP4D_H 1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _QNX_SOURCE

#include <sys/types.h>
#ifdef HAVE_SECURITY_PAM_APPL_H
# include <security/pam_appl.h>
#endif

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <syslog.h>
#include <pwd.h>
#include <grp.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#ifdef HAVE_PATHS_H
# include <paths.h>
#endif
#include <sysexits.h>

#include "xalloc.h"

#include <mailutils/address.h>
#include <mailutils/attribute.h>
#include <mailutils/body.h>
#include <mailutils/daemon.h>
#include <mailutils/envelope.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/filter.h>
#include <mailutils/folder.h>
#include <mailutils/header.h>
#include <mailutils/iterator.h>
#include <mailutils/list.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/mutil.h>
#include <mailutils/mu_auth.h>
#include <mailutils/nls.h>
#include <mailutils/parse822.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/tls.h>
#include <mailutils/url.h>
#include <mailutils/daemon.h>
#include <mailutils/pam.h>
#include <mailutils/acl.h>
#include <mailutils/server.h>

#include <mu_asprintf.h>
#include <mu_umaxtostr.h>
#include <muaux.h>

#ifdef __cplusplus
extern "C" {
#endif

struct imap4d_command
{
  const char *name;
  int (*func) (struct imap4d_command *, char *);
  int states;
  int failure;
  int success;
  char *tag;
};

/* Global variables and constants*/
#define STATE_NONE	(0)
#define STATE_NONAUTH	(1 << 0)
#define STATE_AUTH	(1 << 1)
#define STATE_SEL	(1 << 2)
#define STATE_LOGOUT	(1 << 3)

#define STATE_ALL	(STATE_NONE | STATE_NONAUTH | STATE_AUTH | STATE_SEL \
			| STATE_LOGOUT)

/* Response code.  */
#define RESP_OK		0
#define RESP_BAD	1
#define RESP_NO		2
#define RESP_BYE	3
#define RESP_NONE	4
#define RESP_PREAUTH    5
  
/* Error values.  */
#define OK                    0
#define ERR_NO_MEM            1
#define ERR_NO_OFILE          2
#define ERR_TIMEOUT           3
#define ERR_SIGNAL            4
#define ERR_TLS               5
#define ERR_MAILBOX_CORRUPTED 6
#define ERR_TERMINATE         7
  
/* Namespace numbers */
#define NS_PRIVATE 0
#define NS_OTHER   1
#define NS_SHARED  2
#define NS_MAX     3

/*  IMAP4D capability names */
#define IMAP_CAPA_STARTTLS       "STARTTLS"
#define IMAP_CAPA_LOGINDISABLED  "LOGINDISABLED"
#define IMAP_CAPA_XTLSREQUIRED   "XTLSREQUIRED"  

/* Preauth types */  
enum imap4d_preauth
  {
    preauth_none,
    preauth_stdio,
    preauth_ident,
    preauth_prog
  };

  
extern struct imap4d_command imap4d_command_table[];
extern mu_mailbox_t mbox;
extern char *homedir;
extern char *rootdir;
extern int state;
extern size_t children;
extern int is_virtual;
extern struct mu_auth_data *auth_data; 
extern const char *program_version;
  
extern int login_disabled;
extern int tls_required;
extern enum imap4d_preauth preauth_mode;
extern char *preauth_program;
extern int preauth_only;
extern int ident_port;
extern char *ident_keyfile;
extern int ident_encrypt_only;
extern unsigned int idle_timeout;
extern int imap4d_transcript;
  
#ifndef HAVE_STRTOK_R
extern char *strtok_r (char *s, const char *delim, char **save_ptr);
#endif
  
/* Imap4 commands */
extern int  imap4d_append (struct imap4d_command *, char *);
extern int  imap4d_append0 (mu_mailbox_t mbox, int flags, char *text);
extern int  imap4d_authenticate (struct imap4d_command *, char *);
extern void imap4d_auth_capability (void);
extern int  imap4d_capability (struct imap4d_command *, char *);
extern int  imap4d_check (struct imap4d_command *, char *);
extern int  imap4d_close (struct imap4d_command *, char *);
extern int  imap4d_copy (struct imap4d_command *, char *);
extern int  imap4d_copy0 (char *, int, char *, size_t);
extern int  imap4d_create (struct imap4d_command *, char *);
extern int  imap4d_delete (struct imap4d_command *, char *);
extern int  imap4d_examine (struct imap4d_command *, char *);
extern int  imap4d_expunge (struct imap4d_command *, char *);
extern int  imap4d_fetch (struct imap4d_command *, char *);
extern int  imap4d_fetch0 (char *, int, char *, size_t);
extern int  imap4d_list (struct imap4d_command *, char *);
extern int  imap4d_lsub (struct imap4d_command *, char *);
extern int  imap4d_login (struct imap4d_command *, char *);
extern int  imap4d_logout (struct imap4d_command *, char *);
extern int  imap4d_noop (struct imap4d_command *, char *);
extern int  imap4d_rename (struct imap4d_command *, char *);
extern int  imap4d_preauth_setup (int fd);
extern int  imap4d_search (struct imap4d_command *, char *);
extern int  imap4d_search0 (char *arg, int isuid, char *replybuf, size_t replysize);
extern int  imap4d_select (struct imap4d_command *, char *);
extern int  imap4d_select0 (struct imap4d_command *, char *, int);
extern int  imap4d_select_status (void);
#ifdef WITH_TLS
extern int  imap4d_starttls (struct imap4d_command *, char *);
extern void starttls_init (void);
#endif /* WITH_TLS */
extern int  imap4d_status (struct imap4d_command *, char *);
extern int  imap4d_store (struct imap4d_command *, char *);
extern int  imap4d_store0 (char *, int, char *, size_t);
extern int  imap4d_subscribe (struct imap4d_command *, char *);
extern int  imap4d_uid (struct imap4d_command *, char *);
extern int  imap4d_unsubscribe (struct imap4d_command *, char *);
extern int  imap4d_namespace (struct imap4d_command *, char *);
extern int  imap4d_version (struct imap4d_command *, char *);
extern int  imap4d_idle (struct imap4d_command *, char *);
  
extern int imap4d_check_home_dir (const char *dir, uid_t uid, gid_t gid);

/* Shared between fetch and store */  
extern void fetch_flags0 (const char *prefix, mu_message_t msg, int isuid);

/* Synchronisation on simultaneous access.  */
extern int imap4d_sync (void);
extern int imap4d_sync_flags (size_t);
extern size_t uid_to_msgno (size_t);
extern void imap4d_set_observer (mu_mailbox_t mbox);
  
/* Signal handling.  */
extern RETSIGTYPE imap4d_master_signal (int);
extern RETSIGTYPE imap4d_child_signal (int);
extern int imap4d_bye (int);
extern int imap4d_bye0 (int reason, struct imap4d_command *command);

/* Namespace functions */
extern int set_namespace (int i, char *str);
extern int namespace_init (char *path);
extern char * namespace_getfullpath (char *name, const char *delim);
extern char * namespace_checkfullpath (char *name, const char *pattern,
				       const char *delim);
int imap4d_session_setup (char *username);
int imap4d_session_setup0 (void);
  
/* Capability functions */
extern void imap4d_capability_add (const char *str);
extern void imap4d_capability_remove (const char *str);
extern void imap4d_capability_init (void);
  
/* Helper functions.  */
extern int  util_out (int, const char *, ...) MU_PRINTFLIKE(2,3);
extern int  util_send (const char *, ...) MU_PRINTFLIKE(1,2);
extern int  util_send_qstring (const char *);
extern int  util_send_literal (const char *);
extern int  util_start (char *);
extern int  util_finish (struct imap4d_command *, int, const char *, ...) 
                         MU_PRINTFLIKE(3,4);
extern int  util_getstate (void);
extern int  util_do_command (char *);
extern char *imap4d_readline (void);
extern char *imap4d_readline_ex (void);
extern char *util_getword (char *, char **);
extern char *util_getitem (char *, const char *, char **);
extern int  util_token (char *, size_t, char **);
extern void util_unquote (char **);
extern char *util_tilde_expansion (const char *, const char *);
extern char *util_getfullpath (char *, const char *);
extern int  util_msgset (char *, size_t **, int *, int);
extern int  util_upper (char *);
extern struct imap4d_command *util_getcommand (char *, 
                                               struct imap4d_command []);
extern int util_parse_internal_date0 (char *date, time_t *timep, char **endp);
extern int util_parse_internal_date (char *date, time_t *timep);
extern int util_parse_822_date (const char *date, time_t *timep);
extern int util_parse_ctime_date (const char *date, time_t *timep);
extern char *util_strcasestr (const char *haystack, const char *needle);
extern int util_parse_attributes (char *items, char **save, int *flags);

extern int util_base64_encode (const unsigned char *input,
			       size_t input_len,
			       unsigned char **output,
			       size_t *output_len);
extern int util_base64_decode (const unsigned char *input,
			       size_t input_len,
			       unsigned char **output,
			       size_t *output_len);
extern char *util_localname (void);

extern int util_wcard_match (const char *string, const char *pattern,
			     const char *delim);
void util_print_flags (mu_attribute_t attr);
int util_attribute_to_type (const char *item, int *type);
int util_type_to_attribute (int type, char **attr_str);
int util_attribute_matches_flag (mu_attribute_t attr, const char *item);
int util_uidvalidity (mu_mailbox_t smbox, unsigned long *uidvp);

void util_setio (FILE*, FILE*);
void util_flush_output (void);
void util_get_input (mu_stream_t *pstr);
void util_get_output (mu_stream_t *pstr);
void util_set_input (mu_stream_t str);
void util_set_output (mu_stream_t str);
int util_wait_input (int);
  
void util_register_event (int old_state, int new_state,
			  mu_list_action_t *action, void *data);
void util_event_remove (void *id);
void util_run_events (int old_state, int new_state);
  
int util_is_master (void);
void util_bye (void);  
void util_atexit (void (*fp) (void));
void util_chdir (const char *homedir);
int is_atom (const char *s);
  
#ifdef WITH_TLS
int imap4d_init_tls_server (void);
#endif /* WITH_TLS */

typedef int (*imap4d_auth_handler_fp) (struct imap4d_command *,
				       char *, char *, char **);
  
extern void auth_add (char *name, imap4d_auth_handler_fp handler);
extern void auth_remove (char *name);

#ifdef WITH_GSSAPI  
extern void auth_gssapi_init (void);
#else
# define auth_gssapi_init()  
#endif

#ifdef WITH_GSASL
extern void auth_gsasl_init (void);
#else
# define auth_gsasl_init()
#endif
  
#ifdef __cplusplus
}
#endif

#endif /* _IMAP4D_H */
