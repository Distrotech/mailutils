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
  
  param.debug_guile = 1 /*FIXME*/;
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

SCM
mda_catch_body (void *data, mailbox_t mbox)
{
  struct mda_data *md = data;

  if (access (md->progfile, R_OK))
    {
      syslog (LOG_ERR, "access to %s failed: %m", md->progfile);
    }
  else
    scm_primitive_load (scm_makfrom0str (md->progfile));
  mda (md->fp, md->argv[0], mbox);
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
