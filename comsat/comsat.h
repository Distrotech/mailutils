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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <syslog.h>
#include <string.h>
#include <pwd.h>
#include <ctype.h>

#ifdef HAVE_PATHS_H
# include <paths.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/argp.h>
#include <mailutils/body.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/header.h>
#include <mailutils/list.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/mutil.h>
#include <mailutils/registrar.h>
#include <mailutils/stream.h>
#include <mailutils/mu_auth.h>
#include <mailutils/argcv.h>

#ifndef INADDR_NONE
# define INADDR_NONE -1
#endif

#define BIFF_RC ".biffrc"

extern int allow_biffrc;
extern unsigned maxrequests;
extern time_t request_control_interval;
extern time_t overflow_control_interval;
extern time_t overflow_delay_time;
extern int maxlines;
extern const char *username;
extern char hostname[];
extern struct daemon_param daemon_param;

extern void read_config (const char *config_file);
int acl_match (struct sockaddr_in *sa_in);
void run_user_action (FILE *tty, const char *cr, message_t msg);
