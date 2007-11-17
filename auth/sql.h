/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2005, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

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
#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE
#endif
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

struct mu_internal_sql_config
{
  int interface;
  char *getpwnam_query;
  char *getpass_query;
  char *getpwuid_query;
  char *host; 
  char *user;
  char *passwd;
  char *db;
  int port;
  enum mu_password_type password_type;
  mu_assoc_t field_map;
};

extern struct mu_internal_sql_config mu_sql_module_config;

