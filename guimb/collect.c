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
size_t nmesg;
size_t current_mesg_no;
message_t current_message;

/* Open temporary file for collecting incoming message */
void
collect_open_mailbox_file ()
{
  int fd;

  /* Create input mailbox */
  fd = util_tempfile (&temp_filename);
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
  size_t count;

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

/* Close the temporary mailbox and unlink the file associated with it */
void
collect_drop_mailbox ()
{
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  unlink (temp_filename);
  free (temp_filename);
}
