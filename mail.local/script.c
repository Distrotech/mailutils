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

#include <mail.local.h>

#ifdef WITH_GUILE
#include <mu_scm.h>

int debug_guile;

SCM mda_catch_body (void *data, mailbox_t mbox);
SCM mda_catch_handler (void *unused, SCM tag, SCM throw_args);
int mda_next (void *data, mailbox_t mbox);
int mda_exit (void *data, mailbox_t mbox);
int mda_init (void *data);

int
prog_mda (struct mda_data *data)
{
  char *x_argv[2];
  guimb_param_t param;
  mailbox_t mbox;
      
  x_argv[0] = "mail.local";
  x_argv[1] = NULL;

  fflush (data->fp);
  if (mailbox_create (&mbox, data->tempfile)
      || mailbox_open (mbox, MU_STREAM_RDWR) != 0)
    {
      mailer_err ("can't open temporary storage");
      return EX_UNAVAILABLE;
    }

  unlink (data->tempfile);
  
  param.debug_guile = debug_guile;
  param.mbox = mbox;
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
  md->progfile = make_progfile_name (md->progfile_pattern, md->argv[0]);
  return 0;
}

static void
mda_switch_to_user (struct mda_data *md)
{
  struct passwd *pw = NULL;
  
  if (md && *md->argv != NULL)
    pw = mu_getpwnam (*md->argv);

  if (pw)
    {
      switch_user_id (pw->pw_uid);
      chdir (pw->pw_dir);
    }
  else
    {
      switch_user_id (0);
      chdir ("/");
    }
}

SCM
mda_catch_body (void *data, mailbox_t mbox)
{
  struct mda_data *md = data;
  message_t mesg = NULL;
  attribute_t attr = NULL;
  FILE *fp = md->fp;

  if (access (md->progfile, R_OK))
    {
      if (debug_level > 2)
	syslog (LOG_DEBUG, "access to %s failed: %m", md->progfile);
    }
  else
    {
      mda_switch_to_user (md);
      scm_primitive_load (scm_makfrom0str (md->progfile));
    }
  
  mailbox_get_message (mbox, 1, &mesg);
  message_get_attribute (mesg, &attr);
  if (attribute_is_deleted (attr))
    return SCM_BOOL_F;

  if (message_is_modified (mesg))
    {
      char *tname;
      int fd = mu_tempfile (NULL, &tname);
      mailbox_t tmp;
      
      close (fd);
      if (mailbox_create (&tmp, tname) == 0
	  && mailbox_open (tmp, MU_STREAM_RDWR) == 0)
	{
	  mailbox_append_message (tmp, mesg);
	  mailbox_close (tmp);
	  mailbox_destroy (&tmp);

	  fp = fopen (tname, "r");
	}
      unlink (tname);
    }

  mda_switch_to_user (NULL);
  mda (fp, md->argv[0]);
  if (fp != md->fp)
    fclose (fp);
  return SCM_BOOL_F;
}

SCM
mda_catch_handler (void *data, SCM tag, SCM throw_args)
{
  exit_code = EX_TEMPFAIL;
  return scm_handle_by_message_noexit ("mail.local", tag, throw_args);
}

int
mda_next (void *data, mailbox_t mbox)
{
  struct mda_data *md = data;
  message_t mesg = NULL;
  attribute_t attr = NULL;

  md->argv++;
  if (*md->argv == NULL)
    return 0;
  if (md->progfile)
    free (md->progfile);
  md->progfile = make_progfile_name (md->progfile_pattern, *md->argv);

  mailbox_get_message (mbox, 1, &mesg);
  message_get_attribute (mesg, &attr);
  attribute_unset_deleted (attr);
  
  return md->progfile != NULL;
}

int
mda_exit (void *data, mailbox_t mbox)
{
  return exit_code;
}

#endif
