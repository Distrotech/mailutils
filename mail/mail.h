/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <argp.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/body.h>

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

/* Global variables */
mailbox_t mbox;
unsigned int cursor;
unsigned int realcursor;
unsigned int total;

/* Functions */
int mail_alias __P((int argc, char **argv));
int mail_alt __P((int argc, char **argv));	/* command alternates */
int mail_cd __P((int argc, char **argv));
int mail_copy __P((int argc, char **argv));
int mail_delete __P((int argc, char **argv));
int mail_discard __P((int argc, char **argv));
int mail_dp __P((int argc, char **argv));
int mail_echo __P((int argc, char **argv));
int mail_edit __P((int argc, char **argv));
int mail_exit __P((int argc, char **argv));
int mail_file __P((int argc, char **argv));
int mail_folders __P((int argc, char **argv));
int mail_followup __P((int argc, char **argv));
int mail_from __P((int argc, char **argv));
int mail_headers __P((int argc, char **argv));
int mail_hold __P((int argc, char **argv));
int mail_help __P((int argc, char **argv));
int mail_if __P((int argc, char **argv));
int mail_list __P((int argc, char **argv));
int mail_send __P((int argc, char **argv));	/* command mail */
int mail_mbox __P((int argc, char **argv));
int mail_next __P((int argc, char **argv));
int mail_pipe __P((int argc, char **argv));
int mail_previous __P((int argc, char **argv));
int mail_printall __P((int argc, char **argv));	/* command Print */
int mail_print __P((int argc, char **argv));
int mail_quit __P((int argc, char **argv));
int mail_relist __P((int argc, char **argv));	/* command Reply */
int mail_reply __P((int argc, char **argv));
int mail_retain __P((int argc, char **argv));
int mail_save __P((int argc, char **argv));
int mail_set __P((int argc, char **argv));
int mail_shell __P((int argc, char **argv));
int mail_size __P((int argc, char **argv));
int mail_source __P((int argc, char **argv));
int mail_top __P((int argc, char **argv));
int mail_touch __P((int argc, char **argv));
int mail_unalias __P((int argc, char **argv));
int mail_undelete __P((int argc, char **argv));
int mail_unset __P((int argc, char **argv));
int mail_visual __P((int argc, char **argv));
int mail_write __P((int argc, char **argv));
int mail_z __P((int argc, char **argv));

int mail_bang __P((int argc, char **argv));	/* command ! */
int mail_eq __P((int argc, char **argv));	/* command = */

int util_get_argcv __P((const char *command, int *argc, char ***argv));
int util_expand_msglist __P((const int argc, char **argv, int **list));
int util_do_command __P((const char *cmd));
int util_msglist_command __P((int (*func)(int, char**), int argc, char **argv));
int* util_command_get __P((char *cmd));
int util_free_argv __P((int argc, char **argv));

#ifdef __cplusplus
}
#endif

#endif /* _MAIL_H */

