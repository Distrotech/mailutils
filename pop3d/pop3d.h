/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003,
   2004, 2005 Free Software Foundation, Inc.

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

#ifndef _POP3D_H
#define _POP3D_H	1

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <mu_dbm.h>
#include <mu_asprintf.h>

/* The implementation */
#define	IMPL		"GNU POP3 Daemon"

/* You can edit the messages the POP server prints out here */

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

/* An error occured when expunging.  */
#define FILE_EXP        "Some deleted messages not removed"

/* Command not permitted when TLS active. */
#define TLS_ACTIVE      "Command not permitted when TLS active"

/* Trying to log in within the minimum login delay interval */
#define LOGIN_DELAY     "Attempt to log in within the minimum login delay interval"

/* APOP password file, without .db or .passwd, which are added based on file
   type automatically */
#define APOP_PASSFILE_NAME "apop"

#ifdef USE_DBM
# define APOP_PASSFILE SYSCONFDIR "/" APOP_PASSFILE_NAME
# define ENABLE_LOGIN_DELAY
#else
# define APOP_PASSFILE SYSCONFDIR "/" APOP_PASSFILE_NAME ".passwd"
# undef ENABLE_LOGIN_DELAY
#endif

#ifdef ENABLE_LOGIN_DELAY
# define LOGIN_STAT_FILE "/var/run/pop3-login"
extern time_t login_delay;
extern char *login_stat_file;
extern int check_login_delay __P((char *username));
extern void update_login_delay __P((char *username));
extern void login_delay_capa __P((void));
#else
# define check_login_delay(u) 0
# define update_login_delay(u)
# define login_delay_capa()
#endif

/* Minimum advertise retention time for messages.  */
extern time_t expire;
extern int expire_on_exit;

#define EXPIRE_NEVER ((time_t)-1)

/* Size of the MD5 digest for APOP */
#define APOP_DIGEST	70

/* Longest legal POP command */
#define POP_MAXCMDLEN	255

/* Buffer size to use for output */
#define BUFFERSIZE	1024

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#define _QNX_SOURCE
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
#include <arpa/inet.h>
#include <netdb.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <ctype.h>
#include <md5.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/argp.h>
#include <mailutils/attribute.h>
#include <mailutils/body.h>
#include <mailutils/daemon.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/header.h>
#include <mailutils/list.h>
#include <mailutils/locker.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/mutil.h>
#include <mailutils/mu_auth.h>
#include <mailutils/nls.h>
#include <mailutils/registrar.h>
#include <mailutils/tls.h>
#include <mailutils/url.h>

/* For Berkley DB2 APOP password file */
#ifdef HAVE_DB_H
#include <db.h>
#endif

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

#ifndef MAXHOSTNAMELEN
/* Maximum length of a hostname (is this defined somewhere else?).  */
/* MAXHOSTNAMELEN is already defined on Solaris.  */
# define MAXHOSTNAMELEN	64
#endif

#define POP3_ATTRIBUTE_DELE 0x0001
#define POP3_ATTRIBUTE_RETR 0x0010

#define INITIAL        -1
#define AUTHORIZATION	0
#define TRANSACTION	1
#define UPDATE		2

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
#define ERR_SIGNAL	11
#define ERR_FILE        12
#define ERR_NO_OFILE    13
#define ERR_TIMEOUT	14
#define ERR_UNKNOWN	15
#define ERR_MBOX_SYNC   16
#define ERR_TLS_ACTIVE  17
#define ERR_TLS_IO      18
#define ERR_LOGIN_DELAY 19

extern mailbox_t mbox;
extern int state;
extern int initial_state;
extern char *username;
extern char *maildir;
extern char *md5shared;
extern size_t children;
extern struct daemon_param daemon_param;
extern int debug_mode;
#ifdef WITH_TLS
extern int tls_available;
extern int tls_done;
#endif /* WITH_TLS */
extern int undelete_on_startup;

extern void pop3d_bye           __P ((void));
extern int pop3d_abquit         __P ((int));
extern int pop3d_apop           __P ((const char *));
extern char *pop3d_apopuser     __P ((const char *));
extern char *pop3d_args         __P ((const char *));
extern int pop3d_auth           __P ((const char *));
extern int pop3d_capa           __P ((const char *));
extern char *pop3d_cmd          __P ((const char *));
extern int pop3d_dele           __P ((const char *));
extern int pop3d_list           __P ((const char *));
extern int pop3d_lock           __P ((void));
extern int pop3d_noop           __P ((const char *));
extern int pop3d_quit           __P ((const char *));
extern int pop3d_retr           __P ((const char *));
extern int pop3d_rset           __P ((const char *));
extern void process_cleanup     __P ((void));

extern RETSIGTYPE pop3d_sigchld __P ((int));
extern RETSIGTYPE pop3d_signal  __P ((int));
extern int pop3d_stat           __P ((const char *));
#ifdef WITH_TLS
extern int pop3d_stls           __P ((const char *));
#endif /* WITH_TLS */
extern int pop3d_top            __P ((const char *));
extern int pop3d_touchlock      __P ((void));
extern int pop3d_uidl           __P ((const char *));
extern int pop3d_user           __P ((const char *));
extern int pop3d_unlock         __P ((void));
extern void pop3d_outf          __P ((const char *fmt, ...));

extern void pop3d_setio         __P ((FILE *in, FILE *out));
extern char *pop3d_readline     __P ((char *, size_t));
extern void pop3d_flush_output  __P ((void));

extern int pop3d_is_master      __P ((void));

extern void pop3d_mark_deleted __P((attribute_t attr));
extern int pop3d_is_deleted __P((attribute_t attr));
extern void pop3d_unset_deleted __P((attribute_t attr));
void pop3d_undelete_all __P((void));

#ifdef WITH_TLS
extern int pop3d_init_tls_server    __P ((void));
extern void pop3d_deinit_tls_server __P ((void));
#endif /* WITH_TLS */

extern void pop3d_mark_retr __P((attribute_t attr));
extern int pop3d_is_retr __P((attribute_t attr));
extern void pop3d_unmark_retr __P((attribute_t attr));

extern void expire_mark_message __P((message_t msg, char **value));


#endif /* _POP3D_H */
