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

#ifndef _IMAP4D_H
#define _IMAP4D_H 1

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
#include <syslog.h>
#include <pwd.h>
#include <grp.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <argp.h>

#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/body.h>

#include <argcv.h>

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
#ifndef Function
typedef int Function ();
#endif

struct imap4d_command {
  char *name;
  Function *func;
  int states;
};

/* Global variables and constants*/
#define STATE_NONE	1<<0
#define STATE_NONAUTH	1<<1
#define STATE_AUTH	1<<2
#define STATE_SEL	1<<3
#define STATE_LOGOUT	1<<4

#define STATE_ALL	(STATE_NONE | STATE_NONAUTH | STATE_AUTH | STATE_SEL \
			| STATE_LOGOUT)

#define TAG_NONE	0
#define TAG_SEQ		1

#define RESP_OK		0
#define RESP_BAD	1
#define RESP_NO		2

extern const struct imap4d_command imap4d_command_table[];

/* Functions */
int imap4d_capability __P ((int argc, char **argv));
int imap4d_noop __P ((int argc, char **argv));
int imap4d_logout __P ((int argc, char **argv));
int imap4d_authenticate __P ((int argc, char **argv));
int imap4d_login __P ((int argc, char **argv));
int imap4d_select __P ((int argc, char **argv));
int imap4d_examine __P ((int argc, char **argv));
int imap4d_create __P ((int argc, char **argv));
int imap4d_delete __P ((int argc, char **argv));
int imap4d_rename __P ((int argc, char **argv));
int imap4d_subscribe __P ((int argc, char **argv));
int imap4d_unsubscribe __P ((int argc, char **argv));
int imap4d_list __P ((int argc, char **argv));
int imap4d_lsub __P ((int argc, char **argv));
int imap4d_status __P ((int argc, char **argv));
int imap4d_append __P ((int argc, char **argv));
int imap4d_check __P ((int argc, char **argv));
int imap4d_close __P ((int argc, char **argv));
int imap4d_expunge __P ((int argc, char **argv));
int imap4d_search __P ((int argc, char **argv));
int imap4d_fetch __P ((int argc, char **argv));
int imap4d_store __P ((int argc, char **argv));
int imap4d_copy __P ((int argc, char **argv));
int imap4d_uid __P ((int argc, char **argv));

int util_out __P ((char *seq, int tag, char *f, ...));
int util_start __P ((char *seq));
int util_finish __P ((int argc, char **argv, int resp, char *f, ...));
int util_getstate __P((void));

#ifdef __cplusplus
}
#endif

#endif /* _IMAP4D_H */

