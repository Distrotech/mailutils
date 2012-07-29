/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2012 Free Software Foundation, Inc.

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
#include <setjmp.h>
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
#include <mailutils/mime.h>
#include <mailutils/util.h>
#include <mailutils/mu_auth.h>
#include <mailutils/nls.h>
#include <mailutils/parse822.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>
#include <mailutils/tls.h>
#include <mailutils/url.h>
#include <mailutils/daemon.h>
#include <mailutils/pam.h>
#include <mailutils/acl.h>
#include <mailutils/server.h>
#include <mailutils/wordsplit.h>
#include <mailutils/alloc.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/io.h>
#include <mailutils/prog.h>
#include <mailutils/imapio.h>
#include <mailutils/imaputil.h>
#include <mailutils/msgset.h>

#include <mu_umaxtostr.h>
#include <muaux.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct imap4d_tokbuf *imap4d_tokbuf_t;

struct imap4d_session;
  
struct imap4d_command
{
  const char *name;
  int (*func) (struct imap4d_session *, struct imap4d_command *,
	       imap4d_tokbuf_t);
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
#define ERR_NO_IFILE          3
#define ERR_TIMEOUT           4
#define ERR_SIGNAL            5
#define ERR_TLS               6
#define ERR_MAILBOX_CORRUPTED 7
#define ERR_TERMINATE         8
#define ERR_STREAM_CREATE     9

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

  /* TLS modes */
enum tls_mode
  {
    tls_unspecified,
    tls_no,
    tls_ondemand,
    tls_required,
    tls_connection
  };

struct imap4d_session
{
  enum tls_mode tls_mode;
};
  
extern struct imap4d_command imap4d_command_table[];
extern mu_mailbox_t mbox;
extern char *imap4d_homedir;
extern char *real_homedir;  
extern char *modify_homedir;
extern int state;
extern size_t children;
extern int is_virtual;
extern struct mu_auth_data *auth_data; 
extern const char *program_version;
  
extern int mailbox_mode[NS_MAX];
  
extern int login_disabled;
extern enum imap4d_preauth preauth_mode;
extern char *preauth_program;
extern int preauth_only;
extern int ident_port;
extern char *ident_keyfile;
extern int ident_encrypt_only;
extern unsigned int idle_timeout;
extern int imap4d_transcript;
extern mu_list_t imap4d_id_list;
extern int imap4d_argc;                 
extern char **imap4d_argv;
extern jmp_buf child_jmp;

extern int test_mode;
extern int silent_expunge;

int tls_available;

/* Input functions */
extern mu_stream_t iostream;
extern int  io_untagged_response (int, const char *, ...) MU_PRINTFLIKE(2,3);
extern int  io_sendf (const char *, ...) MU_PRINTFLIKE(1,2);
extern int  io_send_bytes (const char *buf, size_t size);
extern int  io_send_qstring (const char *);
extern int  io_send_literal (const char *);
extern int  io_copy_out (mu_stream_t str, size_t size);
extern int  io_completion_response (struct imap4d_command *, int,
                                    const char *, ...) MU_PRINTFLIKE(3,4);
extern int io_stream_completion_response (mu_stream_t str,
					  struct imap4d_command *command,
					  int rc, 
					  const char *format, ...)
                                    MU_PRINTFLIKE(4,5);
void io_getline (char **pbuf, size_t *psize, size_t *pnbytes);
void io_setio (int, int, int);
void io_flush (void);
int io_wait_input (int);
  
imap4d_tokbuf_t imap4d_tokbuf_init (void);
void imap4d_tokbuf_destroy (imap4d_tokbuf_t *tok);
int imap4d_tokbuf_argc (imap4d_tokbuf_t tok);
char *imap4d_tokbuf_getarg (imap4d_tokbuf_t tok, int n);
void imap4d_readline (imap4d_tokbuf_t tok);
imap4d_tokbuf_t imap4d_tokbuf_from_string (char *str);

  
#define IMAP4_ARG_TAG     0
#define IMAP4_ARG_COMMAND 1
#define IMAP4_ARG_1       2
#define IMAP4_ARG_2       3
#define IMAP4_ARG_3       4
#define IMAP4_ARG_4       5

  /* Specialized buffer parsing */
struct imap4d_parsebuf
{
  imap4d_tokbuf_t tok;
  int arg;            /* Argument number in tok */
  char *tokptr;       /* Current token */
  size_t tokoff;      /* Offset of next delimiter in tokptr */
  int save_char;      /* Character saved from tokptr[tokoff] */
  char *token;        /* Pointer to the current token */
  const char *delim;  /* Array of delimiters */
  char *peek_ptr;     /* Peeked token */
  jmp_buf errjmp;
  char *err_text;
  void *data;
};

typedef struct imap4d_parsebuf *imap4d_parsebuf_t;
  
void imap4d_parsebuf_exit (imap4d_parsebuf_t pb, char *text);
char *imap4d_parsebuf_peek (imap4d_parsebuf_t pb);
char *imap4d_parsebuf_next (imap4d_parsebuf_t pb, int req);
int imap4d_with_parsebuf (imap4d_tokbuf_t tok, int arg,
			  const char *delim,
			  int (*thunk) (imap4d_parsebuf_t), void *data,
			  char **err_text);
#define imap4d_parsebuf_token(p) ((p)->token)
#define imap4d_parsebuf_data(p) ((p)->data)

  /* Imap4 commands */
extern int  imap4d_append (struct imap4d_session *,
			   struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_authenticate (struct imap4d_session *,
				 struct imap4d_command *, imap4d_tokbuf_t);
extern void imap4d_auth_capability (void);
extern int  imap4d_capability (struct imap4d_session *,
			       struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_check (struct imap4d_session *,
			  struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_close (struct imap4d_session *,
			  struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_unselect (struct imap4d_session *,
			     struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_copy (struct imap4d_session *,
			 struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_copy0 (imap4d_tokbuf_t, int isuid, char **err_text);
extern int  imap4d_create (struct imap4d_session *,
			   struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_delete (struct imap4d_session *,
			   struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_examine (struct imap4d_session *,
			    struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_expunge (struct imap4d_session *,
			    struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_fetch (struct imap4d_session *,
			  struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_fetch0 (imap4d_tokbuf_t tok, int isuid, char **err_text);
extern int  imap4d_list (struct imap4d_session *,
			 struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_login (struct imap4d_session *,
			  struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_logout (struct imap4d_session *,
			   struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_noop (struct imap4d_session *,
			 struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_rename (struct imap4d_session *,
			   struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_preauth_setup (int fd);
extern int  imap4d_search (struct imap4d_session *,
			     struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_search0 (imap4d_tokbuf_t, int isuid, char **repyptr);
extern int  imap4d_select (struct imap4d_session *,
			   struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_select0 (struct imap4d_command *, const char *, int);
extern int  imap4d_select_status (void);
#ifdef WITH_TLS
extern int  imap4d_starttls (struct imap4d_session *,
			     struct imap4d_command *, imap4d_tokbuf_t);
extern void starttls_init (void);
void tls_encryption_on (struct imap4d_session *);
#endif /* WITH_TLS */
extern int  imap4d_status (struct imap4d_session *,
			   struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_store (struct imap4d_session *,
			  struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_store0 (imap4d_tokbuf_t, int, char **);

mu_property_t open_subscription (void);
extern int  imap4d_subscribe (struct imap4d_session *,
			      struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_unsubscribe (struct imap4d_session *,
				struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_lsub (struct imap4d_session *,
			 struct imap4d_command *, imap4d_tokbuf_t);

extern int  imap4d_uid (struct imap4d_session *,
			struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_namespace (struct imap4d_session *,
			      struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_version (struct imap4d_session *,
			    struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_idle (struct imap4d_session *,
			 struct imap4d_command *, imap4d_tokbuf_t);
extern int  imap4d_id (struct imap4d_session *,
		       struct imap4d_command *, imap4d_tokbuf_t);
  
extern int imap4d_check_home_dir (const char *dir, uid_t uid, gid_t gid);

/* Shared between fetch and store */  
extern void fetch_flags0 (const char *prefix, mu_message_t msg, int isuid);

/* Permissions for creating intermediate directories.
   FIXME: These should better be configurable. */
#define MKDIR_PERMISSIONS 0700
int make_interdir (const char *name, int delim, int perms);

/* Synchronisation on simultaneous access.  */
extern int imap4d_sync (void);
extern void imap4d_sync_invalidate (void);
extern int imap4d_sync_flags (size_t);
extern size_t uid_to_msgno (size_t);
extern void imap4d_set_observer (mu_mailbox_t mbox);
  
/* Signal handling.  */
extern RETSIGTYPE imap4d_master_signal (int);
extern RETSIGTYPE imap4d_child_signal (int);
extern int imap4d_bye (int);
extern int imap4d_bye0 (int reason, struct imap4d_command *command);
void imap4d_enter_critical (void);
void imap4d_leave_critical (void);


/* Namespace functions */
extern mu_list_t namespace[NS_MAX];
  
extern int namespace_init_session (char *path);
extern void namespace_init (void);
extern char *namespace_getfullpath (const char *name, int *pns);
extern char *namespace_checkfullpath (const char *name, const char *pattern,
				      int *pns);
int imap4d_session_setup (char *username);
int imap4d_session_setup0 (void);
void imap4d_child_signal_setup (RETSIGTYPE (*handler) (int signo));
  
/* Capability functions */
extern void imap4d_capability_add (const char *str);
extern void imap4d_capability_remove (const char *str);
extern void imap4d_capability_init (void);

/* Helper functions.  */

extern int  util_start (char *);
extern int  util_getstate (void);
extern int  util_do_command (struct imap4d_session *, imap4d_tokbuf_t);
extern char *util_getfullpath (const char *);
extern struct imap4d_command *util_getcommand (char *, 
                                               struct imap4d_command []);

enum datetime_parse_mode     /* how to parse date/time strings */
  {
    datetime_default,        /* default mode */
    datetime_date_only,      /* return only date part, ignore time and TZ */
    datetime_time_only       /* return only time and TZ, ignore date */ 
  };

extern int util_parse_internal_date (char *date, time_t *timep,
				     enum datetime_parse_mode flag);
extern int util_parse_822_date (const char *date, time_t *timep,
				enum datetime_parse_mode flag);
extern int util_parse_ctime_date (const char *date, time_t *timep,
				  enum datetime_parse_mode flag);
extern char *util_strcasestr (const char *haystack, const char *needle);
extern char *util_localname (void);

void util_print_flags (mu_attribute_t attr);
int util_attribute_matches_flag (mu_attribute_t attr, const char *item);
int util_uidvalidity (mu_mailbox_t smbox, unsigned long *uidvp);

  
int util_is_master (void);
void util_bye (void);  
void util_atexit (void (*fp) (void));
void util_chdir (const char *homedir);
int is_atom (const char *s);
int util_isdelim (const char *str);
int util_trim_nl (char *s, size_t len);

int set_xscript_level (int xlev);

#ifdef WITH_TLS
int imap4d_init_tls_server (void);
#endif /* WITH_TLS */

struct imap4d_auth
{
  /* input */
  struct imap4d_command *command;
  char *auth_type;
  /* output */
  char *username;
  int response;
};

enum imap4d_auth_result
  {
    imap4d_auth_nosup,
    imap4d_auth_ok,
    imap4d_auth_resp,
    imap4d_auth_fail
  };
  
typedef enum imap4d_auth_result
          (*imap4d_auth_handler_fp) (struct imap4d_auth *);
  
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

/* Quota support */
void quota_setup (void);
int quota_check (mu_off_t size);
void quota_update (mu_off_t size);

#ifdef __cplusplus
}
#endif

#endif /* _IMAP4D_H */
