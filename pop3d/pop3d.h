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

#ifndef _POP3D_H
#define _POP3D_H	1
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* The implementation */
#define	IMPL		"GNU POP3 Daemon"

/* You can edit the messages the POP server prints out here */

/* Initial greeting */
#define WELCOME		"Welcome to " IMPL " (" PACKAGE " " VERSION ")"

/* A command that doesn't exist */
#define BAD_COMMAND	"Invalid command"

/* Incorrect number of arguments passed to a command */
#define BAD_ARGS	"Invalid arguments"

/* Command issued in wrong state */
#define BAD_STATE	"Incorrect state"

/* An action on a message that doesn't exist */
#define NO_MESG		"No such message"

/* An action on a message that doesn't exist */
#define MESG_DELE	"Message has been deleted"

/* A command that is known but not implemented */
#define NOT_IMPL	"Not implemented"

/* Invalid username or password */
#define BAD_LOGIN	"Bad login"

/* User authenticated, but mailbox is locked */
#define MBOX_LOCK	"Mailbox in use"

/* The command argument was > 40 characters */
#define TOO_LONG	"Argument too long"

/* APOP password file, without .db or .passwd, which are added based on file
   type automatically */
#define APOP_PASSFILE	"/etc/apop"

/* Size of the MD5 digest for APOP */
#define APOP_DIGEST	70

/* Longest legal POP command */
#define POP_MAXCMDLEN	255

/* Buffer size to use for output */
#define BUFFERSIZE	1024

#define _GNU_SOURCE
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <ctype.h>
#include "md5.h"
#include "getopt.h"

#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/registrar.h>

/* For Berkley DB2 APOP password file */
#ifdef HAVE_DB_H
#include <db.h>
#endif

/* The path to the mail spool files */
#ifdef HAVE_PATHS_H
#include <paths.h>
#else
#define _PATH_MAILDIR	"/usr/spool/mail"
#endif

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

#ifndef MAXHOSTNAMELEN
/* Maximum length of a hostname (is this defined somewhere else?).  */
/* MAXHOSTNAMELEN is already define on Solaris.  */
#define MAXHOSTNAMELEN	64
#endif


#define AUTHORIZATION	0
#define TRANSACTION	1
#define UPDATE		2

#define INTERACTIVE	0
#define DAEMON		1

#define OK		0
#define ERR_WRONG_STATE	1
#define ERR_BAD_ARGS	2
#define ERR_BAD_LOGIN	3
#define ERR_NO_MESG	4
#define ERR_MESG_DELE   5
#define ERR_NOT_IMPL	6
#define ERR_BAD_CMD	7
#define ERR_MBOX_LOCK	8
#define ERR_TOO_LONG	9
#define ERR_NO_MEM	10
#define ERR_DEAD_SOCK	11
#define ERR_SIGNAL	12
#define ERR_FILE        13
#define ERR_NO_OFILE    14
#define ERR_TIMEOUT	15
#define ERR_UNKNOWN	16

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /* __P */

extern mailbox_t mbox;

extern unsigned int port;
extern unsigned int timeout;
extern int state;
extern char *username;
extern int ifile;
extern FILE *ofile;
extern time_t curr_time;
extern char *md5shared;
extern unsigned int children;

extern int pop3_dele         __P ((const char *arg));
extern int pop3_list         __P ((const char *arg));
extern int pop3_noop         __P ((const char *arg));
extern int pop3_quit         __P ((const char *arg));
extern int pop3_retr         __P ((const char *arg));
extern int pop3_rset         __P ((const char *arg));
extern int pop3_stat         __P ((const char *arg));
extern int pop3_top          __P ((const char *arg));
extern int pop3_uidl         __P ((const char *arg));
extern int pop3_user         __P ((const char *arg));
extern int pop3_apop         __P ((const char *arg));
extern int pop3_auth         __P ((const char *arg));
extern int pop3_capa         __P ((const char *arg));
extern char *pop3_args       __P ((const char *cmd));
extern char *pop3_cmd        __P ((const char *cmd));
extern int pop3_mesg_exist   __P ((int mesg));
extern int pop3_abquit       __P ((int reason));
extern int pop3_lock         __P ((void));
extern int pop3_unlock       __P ((void));
extern int pop3_getsizes     __P ((void));
extern int pop3_mainloop     __P ((int infile, int outfile));
extern void pop3_daemon      __P ((unsigned int maxchildren));
extern void pop3_usage       __P ((char *argv0));
extern void pop3_signal      __P ((int));
extern void pop3_sigchld     __P ((int));
extern void pop3_daemon_init __P ((void));
extern char *pop3_apopuser   __P ((const char *user));
extern char *pop3_readline   __P ((int fd));
#endif /* _POP3D_H */
