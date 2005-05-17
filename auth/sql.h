/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#define _XOPEN_SOURCE
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#ifdef HAVE_CRYPT_H
# include <crypt.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#include <string.h>

#include <mailutils/mu_auth.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/mailbox.h>

extern char *sql_interface;
extern char *mu_sql_getpwnam_query;
extern char *mu_sql_getpass_query;
extern char *mu_sql_getpwuid_query;
extern char *mu_sql_host;
extern char *mu_sql_user; 
extern char *mu_sql_passwd;
extern char *mu_sql_db;
extern char *mu_sql_socket;
extern int  mu_sql_port;


