/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#include "mail.h"

/*
 * e[dit] [msglist]
 */

/* FIXME:  Can this be done by simply calling:
   copy [msglist] tempfile
   ! editor tempfile
*/

int
mail_edit (int argc, char **argv)
{
  message_t msg;
  mailbox_t mbx;
  int *msglist = NULL;
  char *filename;
  char *cmdbuf;
  const char *tmpdir;
  const char *editor;
  int num, i;

  /* Get a temp filename.  */
  tmpdir = getenv ("TMPDIR");
  if (tmpdir == NULL)
    tmpdir = "/tmp"; /* FIXME: use _PATH_TMP */
  filename = tempnam (tmpdir, "mu");
  if (filename == NULL)
    {
      fprintf (stderr, "Unable to create temp file\n");
      return 1;
    }

  mailbox_create (&mbx, filename, 0);
  mailbox_open (mbx, MU_STREAM_WRITE | MU_STREAM_CREAT);

  num = util_expand_msglist (argc, argv, &msglist);
  if (num > 0)
    {
      for (i = 0; i < num; i++)
	{
	  mailbox_get_message (mbox, msglist[i], &msg);
	  mailbox_append_message (mbx, msg);
	}
    }
  else
    {
      mailbox_get_message (mbox, cursor, &msg);
      mailbox_append_message (mbx, msg);
    }

  /* Build the command buffer: ! " " editor " " filename \0  */
  editor = getenv ("EDITOR");
  if (editor == NULL)
    editor = "/bin/vi";
  /* ! +  editor +  space  + filename = len */
  cmdbuf = calloc (strlen (editor) + strlen (filename) + 4, sizeof (char));
  sprintf (cmdbuf, "!%s %s", editor, filename);
  util_do_command (cmdbuf);
  mailbox_close (mbx);
  mailbox_destroy (&mbx);
  remove (filename);
  free (filename);
  free (cmdbuf);
  free (msglist);
  return 0;
}
