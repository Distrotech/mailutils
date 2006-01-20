/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005, 2006 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "imap4d.h"

struct imap_auth {
  char *name;
  imap4d_auth_handler_fp handler;
};

static mu_list_t imap_auth_list;

static int
comp (const void *item, const void *data)
{
  const struct imap_auth *p = item;
  return strcmp (p->name, (const char*) data);
}

void
auth_add (char *name, imap4d_auth_handler_fp handler)
{
  struct imap_auth *p = malloc (sizeof (*p));

  if (!p)
    imap4d_bye (ERR_NO_MEM);

  p->name = name;
  p->handler = handler;
  if (!imap_auth_list)
    {
      mu_list_create (&imap_auth_list);
      mu_list_set_comparator (imap_auth_list, comp);
    }
  mu_list_append (imap_auth_list, (void*)p);
}

void
auth_remove (char *name)
{
  mu_list_remove (imap_auth_list, (void*) name);
}

static int
_auth_capa (void *item, void *usused)
{
  struct imap_auth *p = item;
  util_send(" AUTH=%s", p->name);
  return 0;
}

struct auth_data {
  struct imap4d_command *command;
  char *auth_type;
  char *arg;
  char *username;
  int result;
};

static int
_auth_try (void *item, void *data)
{
  struct imap_auth *p = item;
  struct auth_data *ap = data;

  if (strcmp (p->name, ap->auth_type) == 0)
    {
      ap->result = p->handler (ap->command,
			       ap->auth_type, ap->arg, &ap->username);
      return 1;
    }
  return 0;
}

void
imap4d_auth_capability ()
{
  mu_list_do (imap_auth_list, _auth_capa, NULL);
}

int
imap4d_authenticate (struct imap4d_command *command, char *arg)
{
  char *sp = NULL;
  char *auth_type;
  struct auth_data adata;
  
  auth_type = util_getword (arg, &sp);
  util_unquote (&auth_type);
  if (!auth_type)
    return util_finish (command, RESP_BAD, "Too few arguments");

  if (tls_required)
    return util_finish (command, RESP_NO, "Command disabled: Use STARTTLS first");
  
  adata.command = command;
  adata.auth_type = auth_type;
  adata.arg = sp;
  adata.username = NULL;

  if (mu_list_do (imap_auth_list, _auth_try, &adata) == 0)
    return util_finish (command, RESP_NO,
			"Authentication mechanism not supported");
  
  if (adata.result == RESP_OK && adata.username)
    {
      auth_data = mu_get_auth_by_name (adata.username);
      if (auth_data == NULL)
	return util_finish (command, RESP_NO,
			    "User name or passwd rejected");

      homedir = mu_normalize_path (strdup (auth_data->dir), "/");
      if (imap4d_check_home_dir (homedir, auth_data->uid, auth_data->gid))
	return util_finish (command, RESP_NO,
			    "User name or passwd rejected");
      
      if (auth_data->change_uid)
	setuid (auth_data->uid);

      util_chdir (homedir);
      namespace_init (homedir);
      syslog (LOG_INFO, _("User `%s' logged in"), adata.username);
      
      return util_finish (command, RESP_OK,
			  "%s authentication successful", auth_type);
    }
      
  return util_finish (command, adata.result,
		      "%s authentication failed", auth_type);
}

