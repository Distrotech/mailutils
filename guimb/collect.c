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

#include "guimb.h"

char *temp_filename;
FILE *temp_file;
mailbox_t mbox;

void
collect_open_default ()
{
  size_t nmesg;

  if (!default_mailbox)
    {
      asprintf (&default_mailbox, "%s%s", mu_path_maildir, user_name);
      if (!default_mailbox)
	{
	  util_error ("not enough memory");
	  exit (1);
	}
    }
  if (mailbox_create (&mbox, default_mailbox) != 0
      || mailbox_open (mbox, MU_STREAM_RDWR) != 0)
    {
      util_error ("can't open default mailbox %s: %s",
		  default_mailbox, mu_errstring (errno));
      exit (1);
    }

  /* Suck in the messages */
  mailbox_messages_count (mbox, &nmesg);
}

/* Open temporary file for collecting incoming messages */
void
collect_open_mailbox_file ()
{
  int fd;

  /* Create input mailbox */
  fd = mu_tempfile (NULL, &temp_filename);
  if (fd == -1)
    exit (1);

  temp_file = fdopen (fd, "w");
  if (!temp_file)
    {
      util_error ("fdopen: %s", strerror (errno));
      close (fd);
      exit (1);
    }
}

/* Append contents of file `name' to the temporary file */
int
collect_append_file (char *name)
{
  char *buf = NULL;
  size_t n = 0;
  FILE *fp;

  if (strcmp (name, "-") == 0)
    fp = stdin;
  else
    {
      fp = fopen (name, "r");
      if (!fp)
	{
	  util_error ("can't open input file %s: %s", name, strerror (errno));
	  return -1;
	}
    }

  /* Copy the contents of the file */
  while (getline (&buf, &n, fp) > 0)
    fprintf (temp_file, "%s", buf);

  free (buf);
  fclose (fp);
  return 0;
}

/* Close the temporary, and reopen it as a mailbox. */
void
collect_create_mailbox ()
{
  size_t nmesg;

  if (!temp_file)
    return;
  
  fclose (temp_file);

  if (mailbox_create (&mbox, temp_filename) != 0
      || mailbox_open (mbox, MU_STREAM_READ) != 0)
    {
      util_error ("can't create temp mailbox %s: %s",
		  temp_filename, strerror (errno));
      unlink (temp_filename);
      exit (1);
    }

  /* Suck in the messages */
  mailbox_messages_count (mbox, &nmesg);

  if (nmesg == 0)
    {
      util_error ("input format not recognized");
      exit (1);
    }
}

int
collect_output ()
{
  size_t i, count = 0;
  mailbox_t outbox = NULL;
  int saved_umask;

  if (!temp_filename)
    {
      mailbox_expunge (mbox);
      return 0;
    }

  if (user_name)
    saved_umask = umask (077);
  
  if (mailbox_create_default (&outbox, default_mailbox) != 0
      || mailbox_open (outbox, MU_STREAM_RDWR|MU_STREAM_CREAT) != 0)
    {
      mailbox_destroy (&outbox);
      fprintf (stderr, "guimb: can't open output mailbox %s: %s\n",
	       default_mailbox, strerror (errno));
      return 1;
    }

  mailbox_messages_count (mbox, &count);
  for (i = 1; i <= count; i++)
    {
      message_t msg = NULL;
      attribute_t attr = NULL;

      mailbox_get_message (mbox, i, &msg);
      message_get_attribute (msg, &attr);
      if (!attribute_is_deleted (attr))
	{
	  attribute_set_recent (attr);
	  mailbox_append_message (outbox, msg);
	}
    }

  mailbox_close (outbox);
  mailbox_destroy (&outbox);

  if (user_name)
    umask (saved_umask);
  return 0;
}

  
/* Close the temporary mailbox and unlink the file associated with it */
void
collect_drop_mailbox ()
{
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  if (temp_filename)
    {
      unlink (temp_filename);
      free (temp_filename);
    }
}

SCM
guimb_catch_body (void *data, mailbox_t unused)
{
  struct guimb_data *gd = data;
  if (gd->program_file)
    scm_primitive_load (scm_makfrom0str (gd->program_file));

  if (gd->program_expr)
    scm_eval_0str (gd->program_expr);

  return SCM_BOOL_F;
}

SCM
guimb_catch_handler (void *unused, SCM tag, SCM throw_args)
{
  collect_drop_mailbox ();
  return scm_handle_by_message ("guimb", tag, throw_args);
}

int
guimb_exit (void *unused1, mailbox_t unused2)
{
  int rc = collect_output ();
  collect_drop_mailbox ();
  return rc;
}

