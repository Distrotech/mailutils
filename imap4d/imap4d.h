/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/address.h>
#include <mailutils/registrar.h>

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

struct imap4d_command
{
  const char *name;
  int (*func) __P ((struct imap4d_command *, char *));
  int states;
  int success;
  int failure;
  char *tag;
};

/* Global variables and constants*/
#define STATE_NONE	(1 << 0)
#define STATE_NONAUTH	(1 << 1)
#define STATE_AUTH	(1 << 2)
#define STATE_SEL	(1 << 3)
#define STATE_LOGOUT	(1 << 4)

#define STATE_ALL	(STATE_NONE | STATE_NONAUTH | STATE_AUTH | STATE_SEL \
			| STATE_LOGOUT)

#define RESP_OK		0
#define RESP_BAD	1
#define RESP_NO		2
#define RESP_BYE	3
#define RESP_NONE	4

extern struct imap4d_command imap4d_command_table[];
extern FILE *ofile;
extern unsigned int timeout;
extern mailbox_t mbox;
extern unsigned int state;

/* Imap4 commands */
int imap4d_capability __P ((struct imap4d_command *, char *));
int imap4d_noop __P ((struct imap4d_command *, char *));
int imap4d_logout __P ((struct imap4d_command *, char *));
int imap4d_authenticate __P ((struct imap4d_command *, char *));
int imap4d_login __P ((struct imap4d_command *, char *));
int imap4d_select __P ((struct imap4d_command *, char *));
int imap4d_select0 __P ((struct imap4d_command *, char *, int));
int imap4d_examine __P ((struct imap4d_command *, char *));
int imap4d_create __P ((struct imap4d_command *, char *));
int imap4d_delete __P ((struct imap4d_command *, char *));
int imap4d_rename __P ((struct imap4d_command *, char *));
int imap4d_subscribe __P ((struct imap4d_command *, char *));
int imap4d_unsubscribe __P ((struct imap4d_command *, char *));
int imap4d_list __P ((struct imap4d_command *, char *));
int imap4d_lsub __P ((struct imap4d_command *, char *));
int imap4d_status __P ((struct imap4d_command *, char *));
int imap4d_append __P ((struct imap4d_command *, char *));
int imap4d_check __P ((struct imap4d_command *, char *));
int imap4d_close __P ((struct imap4d_command *, char *));
int imap4d_expunge __P ((struct imap4d_command *, char *));
int imap4d_search __P ((struct imap4d_command *, char *));
int imap4d_fetch __P ((struct imap4d_command *, char *));
int imap4d_store __P ((struct imap4d_command *, char *));
int imap4d_copy __P ((struct imap4d_command *, char *));
int imap4d_uid __P ((struct imap4d_command *, char *));

/* Helper functions.  */
int util_out __P ((int rc, const char *f, ...));
int util_send __P ((const char *f, ...));
int util_start __P ((char *tag));
int util_finish __P ((struct imap4d_command *, int sc, const char *f, ...));
int util_getstate __P ((void));
int util_do_command __P ((char *prompt));
char *imap4d_readline __P ((int fd));
void util_quit __P ((int));
char *util_getword __P ((char *s, char **save_ptr));
int util_token __P ((char *s, size_t, char **save_ptr));
int util_msgset __P ((char *s, int **set, int *n, int isuid));
int util_upper __P ((char *));
struct imap4d_command *util_getcommand __P ((char *cmd,
					     struct imap4d_command []));

#ifdef __cplusplus
}
#endif

#endif /* _IMAP4D_H */
