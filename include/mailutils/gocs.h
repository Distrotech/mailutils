/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_GOCS_H
#define _MAILUTILS_GOCS_H

#include <mailutils/types.h>
#include <mailutils/list.h>

#ifdef __cplusplus
extern "C" { 
#endif

struct mu_gocs_daemon
{
  int mode;
  size_t maxchildren;
  unsigned int port;
  unsigned int timeout;
  int transcript;
  char *pidfile;
};

struct mu_gocs_logging
{
  int facility;
  char *tag;
};
  
struct mu_gocs_mailbox
{
  char *mail_spool;
  char *mailbox_type;
};

struct mu_gocs_locking
{
  char *lock_flags;
  unsigned long lock_retry_timeout;
  unsigned long lock_retry_count;
  unsigned long lock_expire_timeout;
  char *external_locker;
};

struct mu_gocs_source_email
{
  char *address;
  char *domain;
};

struct mu_gocs_mailer
{
  char *mailer;
};

/* Auxiliary variables for use by libargp/libcfg */
extern int mu_load_user_rcfile;
extern int mu_load_site_rcfile;
extern char *mu_load_rcfile;

extern int log_facility; /* FIXME: 1. Belongs elsewhere;
      			           2. Does not begin with `mu_' */

typedef int (*gocs_init_fp) (void *data);

void mu_gocs_register (char *capa, gocs_init_fp init);
void mu_gocs_register_std (char *name);
void mu_gocs_store (char *capa, void *data);
void mu_gocs_flush (void);
int mu_gocs_enumerate (mu_list_action_t action, void *data);

int mu_gocs_mailbox_init (void *data);
int mu_gocs_locking_init (void *data);
int mu_gocs_daemon_init (void *data);
int mu_gocs_source_email_init (void *data);
int mu_gocs_mailer_init (void *data);
int mu_gocs_logging_init (void *data);
  
#ifdef __cplusplus
}
#endif

#endif
