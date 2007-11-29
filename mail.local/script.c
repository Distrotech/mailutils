/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005,
   2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include <mail.local.h>

#ifdef WITH_GUILE
#include <mailutils/guile.h>

int debug_guile;

SCM mda_catch_body (void *data, mu_mailbox_t mbox);
SCM mda_catch_handler (void *unused, SCM tag, SCM throw_args);
int mda_next (void *data, mu_mailbox_t mbox);
int mda_exit (void *data, mu_mailbox_t mbox);
int mda_init (void *data);

int
prog_mda (struct mda_data *data)
{
  char *x_argv[2];
  mu_guimb_param_t param;
  
  x_argv[0] = "mail.local";
  x_argv[1] = NULL;

  param.debug_guile = debug_guile;
  param.mbox = data->mbox;
  param.user_name = NULL;
  param.init = mda_init;
  param.catch_body = mda_catch_body;
  param.catch_handler = mda_catch_handler;
  param.next = mda_next;
  param.exit = mda_exit;
  param.data = data;
  
  mu_process_mailbox (1, x_argv, &param);
  return EX_UNAVAILABLE;
}

int
mda_init (void *data)
{
  struct mda_data *md = data;
  md->progfile = mu_expand_path_pattern (md->progfile_pattern, md->argv[0]);
  return 0;
}

static void
mda_switch_to_user (struct mda_data *md)
{
  struct mu_auth_data *auth = NULL;
  
  if (md && *md->argv != NULL)
    auth = mu_get_auth_by_name (*md->argv);

  if (auth)
    {
      switch_user_id (auth, 1);
      chdir (auth->dir);
      mu_auth_data_free (auth);
    }
  else
    {
      switch_user_id (auth, 0);
      chdir ("/");
    }
}

SCM
mda_catch_body (void *data, mu_mailbox_t mbox)
{
  struct mda_data *md = data;
  mu_message_t mesg = NULL;
  mu_attribute_t attr = NULL;

  if (access (md->progfile, R_OK))
    {
      if (debug_level > 2)
	mu_diag_output (MU_DIAG_DEBUG, _("Access to %s failed: %m"), md->progfile);
    }
  else
    {
      mda_switch_to_user (md);
      scm_primitive_load (scm_makfrom0str (md->progfile));
    }
  
  mu_mailbox_get_message (mbox, 1, &mesg);
  mu_message_get_attribute (mesg, &attr);
  if (mu_attribute_is_deleted (attr))
    return SCM_BOOL_F;

  mda_switch_to_user (NULL);
  mda (md->mbox, md->argv[0]);
  return SCM_BOOL_F;
}

SCM
mda_catch_handler (void *data, SCM tag, SCM throw_args)
{
  exit_code = EX_TEMPFAIL;
  return scm_handle_by_message_noexit ("mail.local", tag, throw_args);
}

int
mda_next (void *data, mu_mailbox_t mbox)
{
  struct mda_data *md = data;
  mu_message_t mesg = NULL;
  mu_attribute_t attr = NULL;

  md->argv++;
  if (*md->argv == NULL)
    return 0;
  if (md->progfile)
    free (md->progfile);
  md->progfile = mu_expand_path_pattern (md->progfile_pattern, *md->argv);

  mu_mailbox_get_message (mbox, 1, &mesg);
  mu_message_get_attribute (mesg, &attr);
  mu_attribute_unset_deleted (attr);
  
  return md->progfile != NULL;
}

int
mda_exit (void *data, mu_mailbox_t mbox)
{
  return exit_code;
}

#endif
