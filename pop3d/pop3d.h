/* GNU POP3 - a small, fast, and efficient POP3 daemon
   Copyright (C) 1999 Jakob 'sparky' Kaivo <jkaivo@nodomainname.net>

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
#include "config.h"

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

/* Maximum length of a hostname (is this defined somewhere else?) */
#define MAXHOSTNAMELEN	64

/* Longest legal POP command */
#define POP_MAXCMDLEN	255

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
#include "mailbox.h"

/* For Berkley DB2 APOP password file */
#ifdef BDB2
#include <db2/db.h>
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
#define ERR_NOT_IMPL	5
#define ERR_BAD_CMD	6
#define ERR_MBOX_LOCK	7
#define ERR_TOO_LONG	8
#define ERR_NO_MEM	9
#define ERR_DEAD_SOCK	10
#define ERR_SIGNAL	11
#define ERR_FILE        12
#define ERR_NO_OFILE    13
#define ERR_TIMEOUT	14

mailbox *mbox;

unsigned int port;
unsigned int timeout;
int state;
char *username;
int ifile;
FILE *ofile;
time_t curr_time;
char *md5shared;

int pop3_dele (const char *arg);
int pop3_list (const char *arg);
int pop3_noop (const char *arg);
int pop3_quit (const char *arg);
int pop3_retr (const char *arg);
int pop3_rset (const char *arg);
int pop3_stat (const char *arg);
int pop3_top (const char *arg);
int pop3_uidl (const char *arg);
int pop3_user (const char *arg);
int pop3_apop (const char *arg);
int pop3_auth (const char *arg);
int pop3_capa (const char *arg);
char *pop3_args (const char *cmd);
char *pop3_cmd (const char *cmd);
int pop3_mesg_exist (int mesg);
int pop3_abquit (int reason);
int pop3_lock (void);
int pop3_unlock (void);
int pop3_getsizes (void);
int pop3_mainloop (int infile, int outfile);
int pop3_daemon (int maxchildren);
void pop3_usage (char *argv0);
void pop3_signal (int signal);
void pop3_daemon_init (void);
#ifdef _USE_APOP
char *pop3_apopuser (const char *user);
#endif
char *pop3_readline (int fd);

#endif /* _POP3D_H */
