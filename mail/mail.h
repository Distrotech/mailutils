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

#ifndef _MAIL_H
#define _MAIL_H 1

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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

#include <argp.h>
#include <xalloc.h>

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/registrar.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/address.h>
#include <mailutils/mutil.h>
#include <mailutils/filter.h>

#include <argcv.h>
#include <getline.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

/* Type definitions */
#ifndef function_t
typedef int function_t ();
#endif

/* Values for mail_command_entry.flags */
#define EF_REG  0x00    /* Regular command */
#define EF_FLOW 0x01    /* Flow control command */
#define EF_SEND 0x02    /* Send command */

struct mail_command_entry {
  char *shortname;
  char *longname;
  int flags;
  function_t *func;
  char *synopsis;
};

struct mail_env_entry {
  char *var;
  int set;
  char *value;
};

struct send_environ
{
  char *to;
  char *cc;
  char *bcc;
  char *subj;
  int done;
  char *filename;
  FILE *file;
  FILE *ofile;
  char **outfiles;
  int nfiles;
};

typedef struct message_set msgset_t;

struct message_set
{
  msgset_t *next;       /* Link to the next message set */
  int npart;            /* Number of parts in this set */
  int *msg_part;        /* Array of part numbers: msg_part[0] is the message
			   number */
};
  
/* Global variables and constants*/
extern mailbox_t mbox;
extern unsigned int cursor;
extern unsigned int realcursor;
extern unsigned int total;
extern FILE *ofile;
extern int interactive;
extern const struct mail_command_entry mail_command_table[];
extern const struct mail_command_entry mail_escape_table[];

/* Functions */
int mail_alias __P((int argc, char **argv));
int mail_alt __P((int argc, char **argv));	/* command alternates */
int mail_cd __P((int argc, char **argv));
int mail_copy __P((int argc, char **argv));
int mail_decode __P((int argc, char **argv));
int mail_delete __P((int argc, char **argv));
int mail_discard __P((int argc, char **argv));
int mail_dp __P((int argc, char **argv));
int mail_echo __P((int argc, char **argv));
int mail_edit __P((int argc, char **argv));
int mail_else __P((int argc, char **argv));
int mail_endif __P((int argc, char **argv));
int mail_exit __P((int argc, char **argv));
int mail_file __P((int argc, char **argv));
int mail_folders __P((int argc, char **argv));
int mail_followup __P((int argc, char **argv));
int mail_from __P((int argc, char **argv));
int mail_headers __P((int argc, char **argv));
int mail_hold __P((int argc, char **argv));
int mail_help __P((int argc, char **argv));
int mail_if __P((int argc, char **argv));
int mail_inc __P((int argc, char **argv));
int mail_list __P((int argc, char **argv));
int mail_send __P((int argc, char **argv));	/* command mail */
int mail_mbox __P((int argc, char **argv));
int mail_next __P((int argc, char **argv));
int mail_pipe __P((int argc, char **argv));
int mail_previous __P((int argc, char **argv));
int mail_print __P((int argc, char **argv));
int mail_quit __P((int argc, char **argv));
int mail_reply __P((int argc, char **argv));
int mail_retain __P((int argc, char **argv));
int mail_save __P((int argc, char **argv));
int mail_set __P((int argc, char **argv));
int mail_shell __P((int argc, char **argv));
int mail_size __P((int argc, char **argv));
int mail_source __P((int argc, char **argv));
int mail_summary __P((int argc, char **argv));
int mail_top __P((int argc, char **argv));
int mail_touch __P((int argc, char **argv));
int mail_unalias __P((int argc, char **argv));
int mail_undelete __P((int argc, char **argv));
int mail_unset __P((int argc, char **argv));
int mail_version __P((int argc, char **argv));
int mail_visual __P((int argc, char **argv));
int mail_warranty __P((int argc, char **argv));
int mail_write __P((int argc, char **argv));
int mail_z __P((int argc, char **argv));
int mail_eq __P((int argc, char **argv));	/* command = */

int if_cond __P((void));

void mail_mainloop __P((char *(*input) __P((void *, int)), void *closure, int do_history));
int mail_copy0 __P((int argc, char **argv, int mark));
int mail_send0 __P((struct send_environ *env, int save_to));
void free_env_headers __P((struct send_environ *env));

/*void print_message __P((message_t mesg, char *prefix, int all_headers, FILE *file));*/

int mail_mbox_commit __P((void));
int mail_is_my_name __P((char *name));
void mail_set_my_name __P((char *name));
char *mail_whoami __P((void));
int mail_header_is_visible __P((char *str));
int mail_mbox_close __P((void));

int var_shell __P((int argc, char **argv, struct send_environ *env));
int var_command __P((int argc, char **argv, struct send_environ *env));
int var_help __P((int argc, char **argv, struct send_environ *env));
int var_sign __P((int argc, char **argv, struct send_environ *env));
int var_bcc __P((int argc, char **argv, struct send_environ *env));
int var_cc __P((int argc, char **argv, struct send_environ *env));
int var_deadletter __P((int argc, char **argv, struct send_environ *env));
int var_editor __P((int argc, char **argv, struct send_environ *env));
int var_print __P((int argc, char **argv, struct send_environ *env));
int var_headers __P((int argc, char **argv, struct send_environ *env));
int var_insert __P((int argc, char **argv, struct send_environ *env));
int var_quote __P((int argc, char **argv, struct send_environ *env));
int var_type_input __P((int argc, char **argv, struct send_environ *env));
int var_read __P((int argc, char **argv, struct send_environ *env));
int var_subj __P((int argc, char **argv, struct send_environ *env));
int var_to __P((int argc, char **argv, struct send_environ *env));
int var_visual __P((int argc, char **argv, struct send_environ *env));
int var_write __P((int argc, char **argv, struct send_environ *env));
int var_exit __P((int argc, char **argv, struct send_environ *env));
int var_pipe __P((int argc, char **argv, struct send_environ *env));

/* msgsets */
void msgset_free __P((msgset_t *msg_set));
msgset_t *msgset_make_1 __P((int number));
msgset_t *msgset_append __P((msgset_t *one, msgset_t *two));
msgset_t *msgset_range __P((int low, int high));
msgset_t *msgset_expand __P((msgset_t *set, msgset_t *expand_by));
msgset_t * msgset_dup __P((const msgset_t *set));
int parse_msgset __P((const int argc, char **argv, msgset_t **mset));

int util_do_command __P((const char *cmd, ...));
int util_msglist_command __P((function_t *func, int argc, char **argv, int set_cursor));
function_t* util_command_get __P((char *cmd));
char *util_stripwhite __P((char *string));
struct mail_command_entry util_find_entry __P((const struct mail_command_entry *table, char *cmd));
int util_getcols __P((void));
int util_getlines __P((void));
int util_screen_lines __P((void));
int util_screen_columns __P((void));
struct mail_env_entry *util_find_env __P((const char *var));
int util_printenv __P((int set));
int util_setenv __P((const char *name, const char *value, int overwrite));
int util_isdeleted __P((int message));
char *util_get_homedir __P((void));
char *util_fullpath __P((char *inpath));
char *util_get_sender __P((int msgno, int strip));

void util_slist_print __P((list_t list, int nl));
int util_slist_lookup __P((list_t list, char *str));
void util_slist_add __P((list_t *list, char *value));
void util_slist_destroy __P((list_t *list));
char *util_slist_to_string __P((list_t list, char *delim));
void util_strcat __P((char **dest, char *str));
void util_strupper __P((char *str));
void util_escape_percent __P((char **str));
char *util_outfolder_name __P((char *str));
void util_save_outgoing __P((message_t msg, char *savefile));
void util_error __P((const char *format, ...));
int util_help __P((const struct mail_command_entry *table, char *word));
int util_tempfile __P((char **namep));
void util_msgset_iterate __P((msgset_t *msgset, int (*fun)(), void *closure));
int util_get_content_type __P((header_t hdr, char **value));

int ml_got_interrupt __P((void));
void ml_clear_interrupt __P((void));
void ml_readline_init __P((void));
int ml_reread __P((char *prompt, char **text));

char *alias_expand __P((char *name));
void alias_destroy __P((char *name));

#ifndef HAVE_READLINE_READLINE_H
char *readline __P((const char *prompt));
#endif

#ifndef _PATH_SENDMAIL
#define _PATH_SENDMAIL "/usr/lib/sendmail"
#endif

/* Message attributes */
#define MAIL_ATTRIBUTE_MBOXED   0x0001
#define MAIL_ATTRIBUTE_SAVED    0x0002

#ifdef __cplusplus
}
#endif

#endif /* _MAIL_H */

