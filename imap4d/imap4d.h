/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _IMAP4D_H
#define _IMAP4D_H 1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _QNX_SOURCE
#define _GNU_SOURCE

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
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

#include <mailutils/address.h>
#include <mailutils/argp.h>
#include <mailutils/attribute.h>
#include <mailutils/body.h>
#include <mailutils/envelope.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/filter.h>
#include <mailutils/folder.h>
#include <mailutils/header.h>
#include <mailutils/list.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/mutil.h>
#include <mailutils/parse822.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/mu_auth.h>
#include <mailutils/url.h>
#include <mailutils/nls.h>

#ifdef __cplusplus
extern "C" {
#endif

struct imap4d_command
{
  const char *name;
  int (*func) __P ((struct imap4d_command *, char *));
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

/* Error values.  */
#define OK 0
#define ERR_NO_MEM 1
#define ERR_NO_OFILE 2
#define ERR_TIMEOUT 3
#define ERR_SIGNAL 4
  
/* Namespace numbers */
#define NS_PRIVATE 0
#define NS_OTHER   1
#define NS_SHARED  2
#define NS_MAX     3

/* Wildcard return codes */
#define WCARD_NOMATCH        0
#define WCARD_MATCH          1
#define WCARD_RECURSE_MATCH  2
       
extern struct imap4d_command imap4d_command_table[];
extern FILE *ifile;
extern FILE *ofile;
extern mailbox_t mbox;
extern char *homedir;
extern char *rootdir;
extern int state;
extern volatile size_t children;
extern int is_virtual;
extern struct daemon_param daemon_param;
extern struct mu_auth_data *auth_data; 
	
#ifndef HAVE_STRTOK_R
extern char *strtok_r __P((char *s, const char *delim, char **save_ptr));
#endif
  
/* Imap4 commands */
extern int  imap4d_append __P ((struct imap4d_command *, char *));
extern int  imap4d_append0 __P((mailbox_t mbox, int flags, char *text));
extern int  imap4d_authenticate __P ((struct imap4d_command *, char *));
extern void imap4d_auth_capability __P((void));
extern int  imap4d_capability __P ((struct imap4d_command *, char *));
extern int  imap4d_check __P ((struct imap4d_command *, char *));
extern int  imap4d_close __P ((struct imap4d_command *, char *));
extern int  imap4d_copy __P ((struct imap4d_command *, char *));
extern int  imap4d_copy0 __P ((char *, int, char *, size_t));
extern int  imap4d_create __P ((struct imap4d_command *, char *));
extern int  imap4d_delete __P ((struct imap4d_command *, char *));
extern int  imap4d_examine __P ((struct imap4d_command *, char *));
extern int  imap4d_expunge __P ((struct imap4d_command *, char *));
extern int  imap4d_fetch __P ((struct imap4d_command *, char *));
extern int  imap4d_fetch0 __P ((char *, int, char *, size_t));
extern int  imap4d_list __P ((struct imap4d_command *, char *));
extern int  imap4d_lsub __P ((struct imap4d_command *, char *));
extern int  imap4d_login __P ((struct imap4d_command *, char *));
extern int  imap4d_logout __P ((struct imap4d_command *, char *));
extern int  imap4d_noop __P ((struct imap4d_command *, char *));
extern int  imap4d_rename __P ((struct imap4d_command *, char *));
extern int  imap4d_search __P ((struct imap4d_command *, char *));
extern int  imap4d_search0 __P((char *arg, int isuid, char *replybuf, size_t replysize));
extern int  imap4d_select __P ((struct imap4d_command *, char *));
extern int  imap4d_select0 __P ((struct imap4d_command *, char *, int));
extern int  imap4d_select_status __P((void));
extern int  imap4d_status __P ((struct imap4d_command *, char *));
extern int  imap4d_store __P ((struct imap4d_command *, char *));
extern int  imap4d_store0 __P ((char *, int, char *, size_t));
extern int  imap4d_subscribe __P ((struct imap4d_command *, char *));
extern int  imap4d_uid __P ((struct imap4d_command *, char *));
extern int  imap4d_unsubscribe __P ((struct imap4d_command *, char *));
extern int  imap4d_namespace __P ((struct imap4d_command *, char *));
extern int  imap4d_version __P ((struct imap4d_command *, char *));
  
/* Shared between fetch and store */  
extern void fetch_flags0 (const char *prefix, message_t msg, int isuid);

/* Synchronisation on simultaneous access.  */
extern int imap4d_sync __P ((void));
extern int imap4d_sync_flags __P ((size_t));
extern size_t uid_to_msgno __P ((size_t));

/* Signal handling.  */
extern RETSIGTYPE imap4d_sigchld __P ((int));
extern RETSIGTYPE imap4d_signal __P ((int));
extern int imap4d_bye __P ((int));
extern int imap4d_bye0 __P ((int reason, struct imap4d_command *command));

/* Namespace functions */
extern int set_namespace __P((int i, char *str));
extern int namespace_init __P((char *path));
extern char * namespace_getfullpath __P((char *name, const char *delim));
extern char * namespace_checkfullpath __P((char *name, const char *pattern,
					   const char *delim));
  
/* Helper functions.  */
extern int  util_out __P ((int, const char *, ...));
extern int  util_send __P ((const char *, ...));
extern int  util_send_qstring __P ((const char *));
extern int  util_send_literal __P ((const char *));
extern int  util_start __P ((char *));
extern int  util_finish __P ((struct imap4d_command *, int, const char *, ...));
extern int  util_getstate __P ((void));
extern int  util_do_command __P ((char *));
extern char *imap4d_readline __P ((FILE*));
extern char *imap4d_readline_ex __P ((FILE*));
extern char *util_getword __P ((char *, char **));
extern char *util_getitem __P ((char *, const char *, char **));
extern int  util_token __P ((char *, size_t, char **));
extern void util_unquote __P ((char **));
extern char *util_tilde_expansion __P ((const char *, const char *));
extern char *util_getfullpath __P ((char *, const char *));
extern int  util_msgset __P ((char *, size_t **, int *, int));
extern int  util_upper __P ((char *));
extern struct imap4d_command *util_getcommand __P ((char *,
						    struct imap4d_command []));
extern int util_parse_internal_date0 __P((char *date, time_t *timep, char **endp));
extern int util_parse_internal_date __P((char *date, time_t *timep));
extern int util_parse_822_date __P((char *date, time_t *timep));
extern int util_parse_ctime_date __P((const char *date, time_t *timep));
extern char *util_strcasestr __P((const char *haystack, const char *needle));
extern int util_parse_attributes __P((char *items, char **save, int *flags));

extern int util_base64_encode __P((const unsigned char *input,
				   size_t input_len,
				   unsigned char **output,
				   size_t *output_len));
extern int util_base64_decode __P((const unsigned char *input,
				   size_t input_len,
				   unsigned char **output,
				   size_t *output_len));
extern char *util_localname __P((void));

extern int util_wcard_match __P((const char *string, const char *pattern,
				 const char *delim));
void util_print_flags __P((attribute_t attr));
int util_attribute_to_type __P((const char *item, int *type));
int util_type_to_attribute __P((int type, char **attr_str));
int util_attribute_matches_flag __P((attribute_t attr, const char *item));
int util_uidvalidity __P((mailbox_t smbox, unsigned long *uidvp));
  
#ifdef __cplusplus
}
#endif

#endif /* _IMAP4D_H */
