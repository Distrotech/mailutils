/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifndef _MAIL_H
#define _MAIL_H 1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#include <sys/wait.h>
#include <sys/types.h>
#ifdef HAVE_STDARG_H
# include <stdarg.h>
#else
# include <varargs.h>
#endif
#include <signal.h>
#include <ctype.h>
#ifdef HAVE_PATHS_H
# include <paths.h>
#endif

#include <xalloc.h>

#ifdef HAVE_READLINE_READLINE_H
# include <readline/readline.h>
# include <readline/history.h>
#endif

#include <mailutils/address.h>
#include <mailutils/argp.h>
#include <mailutils/attribute.h>
#include <mailutils/body.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/envelope.h>
#include <mailutils/filter.h>
#include <mailutils/header.h>
#include <mailutils/iterator.h>
#include <mailutils/list.h>
#include <mailutils/mailbox.h>
#include <mailutils/mailer.h>
#include <mailutils/message.h>
#include <mailutils/mutil.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>
#include <mailutils/nls.h>
#include <mailutils/tls.h>
#include <mailutils/argcv.h>
#include <getline.h>
#include <mu_asprintf.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Type definitions */
#ifndef function_t
typedef int function_t __P ((int, char **));
#endif

/* Values for mail_command_entry.flags */
#define EF_REG  0x00    /* Regular command */
#define EF_FLOW 0x01    /* Flow control command */
#define EF_SEND 0x02    /* Send command */

typedef struct compose_env
{
  header_t header;   /* The message headers */
  char *filename;    /* Name of the temporary compose file */
  FILE *file;        /* Temporary compose file */
  FILE *ofile;       /* Diagnostics output channel */
  char **outfiles;   /* Names of the output files. The message is to be
		        saved in each of these. */
  int nfiles;        /* Number of output files */
} compose_env_t;

struct mail_command_entry {
  const char *shortname;
  const char *longname;
  const char *synopsis;
  int flags;
  int (*func)    __P ((int, char **));
  int (*escfunc) __P ((int, char **, compose_env_t *));
};

typedef enum {
  Mail_env_whatever,
  Mail_env_number,
  Mail_env_string,
  Mail_env_boolean
} mail_env_data_t;
  
struct mail_env_entry {
  char *var;
  mail_env_data_t type;
  int set;
  union {
    char *string;
    int number;
  } value;
};

#define mail_env_entry_is_set(ep) ((ep) && (ep)->set)
  
typedef struct message_set msgset_t;

struct message_set
{
  msgset_t *next;       /* Link to the next message set */
  unsigned int npart;   /* Number of parts in this set */
  size_t *msg_part;     /* Array of part numbers: msg_part[0] is the 
                           message number */
};

typedef int (*msg_handler_t) __P((msgset_t *mp, message_t mesg, void *data));

/* Global variables and constants*/
extern mailbox_t mbox;
extern unsigned int cursor;
extern size_t total;
extern FILE *ofile;
extern int interactive;
extern const struct mail_command_entry mail_command_table[];
extern const struct mail_command_entry mail_escape_table[];

/* Functions */
extern int mail_alias __P ((int argc, char **argv));
extern int mail_alt __P ((int argc, char **argv));	/* command alternates */
extern int mail_cd __P ((int argc, char **argv));
extern int mail_copy __P ((int argc, char **argv));
extern int mail_decode __P ((int argc, char **argv));
extern int mail_delete __P ((int argc, char **argv));
extern int mail_discard __P ((int argc, char **argv));
extern int mail_dp __P ((int argc, char **argv));
extern int mail_echo __P ((int argc, char **argv));
extern int mail_edit __P ((int argc, char **argv));
extern int mail_else __P ((int argc, char **argv));
extern int mail_endif __P ((int argc, char **argv));
extern int mail_exit __P ((int argc, char **argv));
extern int mail_file __P ((int argc, char **argv));
extern int mail_folders __P ((int argc, char **argv));
extern int mail_followup __P ((int argc, char **argv));
extern int mail_from __P ((int argc, char **argv));
extern int mail_from0 __P((msgset_t *mspec, message_t msg, void *data));
extern int mail_headers __P ((int argc, char **argv));
extern int mail_hold __P ((int argc, char **argv));
extern int mail_help __P ((int argc, char **argv));
extern int mail_if __P ((int argc, char **argv));
extern int mail_inc __P ((int argc, char **argv));
extern int mail_list __P ((int argc, char **argv));
extern int mail_send __P ((int argc, char **argv));	/* command mail */
extern int mail_mbox __P ((int argc, char **argv));
extern int mail_next __P ((int argc, char **argv));
extern int mail_pipe __P ((int argc, char **argv));
extern int mail_previous __P ((int argc, char **argv));
extern int mail_print __P ((int argc, char **argv));
extern int mail_quit __P ((int argc, char **argv));
extern int mail_reply __P ((int argc, char **argv));
extern int mail_retain __P ((int argc, char **argv));
extern int mail_save __P ((int argc, char **argv));
extern int mail_set __P ((int argc, char **argv));
extern int mail_shell __P ((int argc, char **argv));
extern int mail_execute __P((int shell, int argc, char **argv));
extern int mail_size __P ((int argc, char **argv));
extern int mail_source __P ((int argc, char **argv));
extern int mail_summary __P ((int argc, char **argv));
extern int mail_tag __P ((int argc, char **argv));
extern int mail_top __P ((int argc, char **argv));
extern int mail_touch __P ((int argc, char **argv));
extern int mail_unalias __P ((int argc, char **argv));
extern int mail_undelete __P ((int argc, char **argv));
extern int mail_unset __P ((int argc, char **argv));
extern int mail_version __P ((int argc, char **argv));
extern int mail_visual __P ((int argc, char **argv));
extern int mail_warranty __P ((int argc, char **argv));
extern int mail_write __P ((int argc, char **argv));
extern int mail_z __P ((int argc, char **argv));
extern int mail_eq __P ((int argc, char **argv));	/* command = */
extern int mail_setenv __P ((int argc, char **argv));

extern int if_cond __P ((void));

extern void mail_mainloop __P ((char *(*input) __P((void *, int)), void *closure, int do_history));
extern int mail_copy0 __P ((int argc, char **argv, int mark));
extern int mail_send0 __P ((compose_env_t *env, int save_to));
extern void free_env_headers __P ((compose_env_t *env));

/*extern void print_message __P((message_t mesg, char *prefix, int all_headers, FILE *file));*/

extern int mail_mbox_commit __P ((void));
extern int mail_is_my_name __P ((char *name));
extern void mail_set_my_name __P ((char *name));
extern char *mail_whoami __P ((void));
extern int mail_header_is_visible __P ((char *str));
extern int mail_mbox_close __P ((void));
extern char *mail_expand_name __P((const char *name));

extern int var_shell __P ((int argc, char **argv, compose_env_t *env));
extern int var_command __P ((int argc, char **argv, compose_env_t *env));
extern int var_help __P ((int argc, char **argv, compose_env_t *env));
extern int var_sign __P ((int argc, char **argv, compose_env_t *env));
extern int var_bcc __P ((int argc, char **argv, compose_env_t *env));
extern int var_cc __P ((int argc, char **argv, compose_env_t *env));
extern int var_deadletter __P ((int argc, char **argv, compose_env_t *env));
extern int var_editor __P ((int argc, char **argv, compose_env_t *env));
extern int var_print __P ((int argc, char **argv, compose_env_t *env));
extern int var_headers __P ((int argc, char **argv, compose_env_t *env));
extern int var_insert __P ((int argc, char **argv, compose_env_t *env));
extern int var_quote __P ((int argc, char **argv, compose_env_t *env));
extern int var_type_input __P ((int argc, char **argv, compose_env_t *env));
extern int var_read __P ((int argc, char **argv, compose_env_t *env));
extern int var_subj __P ((int argc, char **argv, compose_env_t *env));
extern int var_to __P ((int argc, char **argv, compose_env_t *env));
extern int var_visual __P ((int argc, char **argv, compose_env_t *env));
extern int var_write __P ((int argc, char **argv, compose_env_t *env));
extern int var_exit __P ((int argc, char **argv, compose_env_t *env));
extern int var_pipe __P ((int argc, char **argv, compose_env_t *env));

/* msgsets */
extern void msgset_free __P ((msgset_t *msg_set));
extern msgset_t *msgset_make_1 __P ((int number));
extern msgset_t *msgset_append __P ((msgset_t *one, msgset_t *two));
extern msgset_t *msgset_range __P ((int low, int high));
extern msgset_t *msgset_expand __P ((msgset_t *set, msgset_t *expand_by));
extern msgset_t *msgset_dup __P ((const msgset_t *set));
extern int msgset_parse __P ((const int argc, char **argv,
			      int flags, msgset_t **mset));
extern int msgset_member __P ((msgset_t *set, size_t n));
extern msgset_t *msgset_negate __P((msgset_t *set));
extern size_t msgset_count __P((msgset_t *set));

extern int util_do_command __P ((const char *cmd, ...));

extern int util_foreach_msg __P((int argc, char **argv, int flags,
				 msg_handler_t func, void *data));
extern int util_range_msg __P((size_t low, size_t high, int flags, 
			       msg_handler_t func, void *data));

extern function_t* util_command_get __P ((const char *cmd));
extern char *util_stripwhite __P ((char *string));
extern struct mail_command_entry util_find_entry __P ((const struct mail_command_entry *table, const char *cmd));
extern int util_getcols __P ((void));
extern int util_getlines __P ((void));
extern int util_screen_lines __P ((void));
extern int util_screen_columns __P ((void));
extern int util_get_crt __P((void));
extern struct mail_env_entry *util_find_env __P ((const char *var,
						  int create));
extern int util_getenv __P ((void *ptr, const char *variable,
			     mail_env_data_t type, int warn));

extern int util_printenv __P ((int set));
extern int util_setenv __P ((const char *name, void *value,
			     mail_env_data_t type, int overwrite));
extern int util_isdeleted __P ((size_t msgno));
extern char *util_get_homedir __P ((void));
extern char *util_fullpath __P ((const char *inpath));
extern char *util_folder_path __P((const char *name));
extern char *util_get_sender __P ((int msgno, int strip));

extern void util_slist_print __P ((list_t list, int nl));
extern int util_slist_lookup __P ((list_t list, char *str));
extern void util_slist_add __P ((list_t *list, char *value));
extern void util_slist_destroy __P ((list_t *list));
extern char *util_slist_to_string __P ((list_t list, const char *delim));
extern void util_strcat __P ((char **dest, const char *str));
extern void util_strupper __P ((char *str));
extern void util_escape_percent __P ((char **str));
extern char *util_outfolder_name __P ((char *str));
extern void util_save_outgoing __P ((message_t msg, char *savefile));
extern void util_error __P ((const char *format, ...));
extern int util_error_range __P ((size_t msgno));
extern void util_noapp __P ((void));
extern int util_help __P ((const struct mail_command_entry *table, char *word));
extern int util_tempfile __P ((char **namep));
extern void util_msgset_iterate __P ((msgset_t *msgset, int (*fun) __P ((message_t, msgset_t *, void *)), void *closure));
extern int util_get_content_type __P ((header_t hdr, char **value));
extern int util_get_hdr_value __P ((header_t hdr, const char *name, char **value));
extern int util_merge_addresses __P((char **addr_str, const char *value));
extern int util_header_expand __P((header_t *hdr));
extern int util_get_message __P((mailbox_t mbox, size_t msgno,
				 message_t *msg));
void util_cache_command __P((list_t *list, const char *fmt, ...));
void util_run_cached_commands __P((list_t *list));
const char *util_reply_prefix __P((void));

extern int ml_got_interrupt __P ((void));
extern void ml_clear_interrupt __P ((void));
extern void ml_readline_init __P ((void));
extern int ml_reread __P ((const char *prompt, char **text));
extern char *ml_readline __P((char *prompt));
extern char *ml_readline_with_intr __P((char *prompt));

extern char *alias_expand __P ((char *name));
extern void alias_destroy __P ((char *name));

#define COMPOSE_APPEND      0
#define COMPOSE_REPLACE     1
#define COMPOSE_SINGLE_LINE 2

void compose_init __P((compose_env_t *env));
int compose_header_set __P((compose_env_t *env, char *name,
			    char *value, int replace));
char *compose_header_get __P((compose_env_t *env, char *name,
			      char *defval));
void compose_destroy __P((compose_env_t *env));

#ifndef HAVE_READLINE_READLINE_H
extern char *readline __P ((char *prompt));
#endif

#ifndef _PATH_SENDMAIL
#define _PATH_SENDMAIL "/usr/lib/sendmail"
#endif

/* Flags for util_get_message */
#define MSG_ALL       0
#define MSG_NODELETED 0x0001
#define MSG_SILENT    0x0002

/* Message attributes */
#define MAIL_ATTRIBUTE_MBOXED   0x0001
#define MAIL_ATTRIBUTE_SAVED    0x0002
#define MAIL_ATTRIBUTE_TAGGED   0x0004

#ifdef __cplusplus
}
#endif

#endif /* _MAIL_H */
