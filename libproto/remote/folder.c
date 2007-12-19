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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/url.h>
#include <mailbox0.h>
#include <url0.h>
#include <folder0.h>
#include <registrar0.h>

#define MU_REMOTE_MBOX_PREFIX "remote+"
#define MU_REMOTE_MBOX_PREFIX_LEN (sizeof (MU_REMOTE_MBOX_PREFIX) - 1)

extern int remote_mbox_init (mu_mailbox_t mailbox);

int
remote_url_init (mu_url_t url)
{
  const char *name = mu_url_to_string (url);
  const char *p;
  size_t len = strlen (name);
  int rc;
  
  if (!name)
    return 0;
  /* reject the obvious */
  if (name == NULL
      || len < MU_REMOTE_MBOX_PREFIX_LEN
      || strncmp (MU_REMOTE_MBOX_PREFIX, name, MU_REMOTE_MBOX_PREFIX_LEN) != 0)
    return EINVAL;

  rc = mu_registrar_lookup (name + MU_REMOTE_MBOX_PREFIX_LEN, 0, NULL, NULL);
  if (rc)
    return rc;

  p = strchr (name, ':');
  if (!p)
    return EINVAL;
  p++;

  len = p - name;
  url->scheme = malloc (len + 1);
  if (!url->scheme)
    return ENOMEM;
  memcpy (url->scheme, name, len);
  url->scheme[len] = 0;
  
  return 0;
}

static int
remote_folder_init (mu_folder_t folder MU_ARG_UNUSED)
{
  return 0;
}

static struct _mu_record _remote_mbox_record =
{
  MU_REMOTE_MBOX_PRIO,
  MU_REMOTE_MBOX_PREFIX,
  remote_url_init, /* Mailbox init.  */
  remote_mbox_init, /* Mailbox init.  */
  NULL, /* Mailer init.  */
  remote_folder_init, /* Folder init.  */
  NULL, /* No need for back pointer.  */
  NULL, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL  /* _get_folder method.  */
};
mu_record_t mu_remote_mbox_record = &_remote_mbox_record;
