/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "mail.h"

/*
 * c[opy] [file]
 * c[opy] [msglist] file
 * C[opy] [msglist]
 */

/*
 * mail_copy0() is shared between mail_copy() and mail_save().
 * argc, argv -- argument count & vector
 * mark -- whether we should mark the message as saved.
 */
int
mail_copy0 (int argc, char **argv, int mark)
{
  message_t msg;
  mailbox_t mbx;
  char *filename = NULL;
  msgset_t *msglist = NULL, *mp;
  int sender = 0;
  size_t total_size = 0, total_lines = 0, size;
  int status;

  if (isupper (argv[0][0]))
    sender = 1;
  else if (argc >= 2)
    filename = mail_expand_name (argv[--argc]);
  else
    filename = strdup ("mbox");

  if (msgset_parse (argc, argv, MSG_NODELETED|MSG_SILENT, &msglist))
    {
      if (filename)
	free (filename);
      return 1;
    }

  if (sender)
    filename = util_outfolder_name (util_get_sender (msglist->msg_part[0], 1));

  if (!filename)
    {
      msgset_free (msglist);
      return 1;
    }

  if (mailbox_create_default (&mbx, filename)
      || mailbox_open (mbx, MU_STREAM_WRITE | MU_STREAM_CREAT))
    {
      util_error (_("Cannot create mailbox %s"), filename);
      free (filename);
      msgset_free (msglist);
      return 1;
    }

  for (mp = msglist; mp; mp = mp->next)
    {
      status = util_get_message (mbox, mp->msg_part[0], &msg);
      if (status)
        break;

      status = mailbox_append_message (mbx, msg);
      if (status)
	{
	  util_error (_("Cannot append message: %s"), mu_strerror (status));
	  break;
	}
      
      message_size (msg, &size);
      total_size += size;
      message_lines (msg, &size);
      total_lines += size;

      if (mark)
 	{
	  attribute_t attr;
	  message_get_attribute (msg, &attr);
	  attribute_set_userflag (attr, MAIL_ATTRIBUTE_SAVED);
	}
    }

  if (status == 0)
    fprintf (ofile, "\"%s\" %3d/%-5d\n", filename, total_lines, total_size);

  mailbox_close (mbx);
  mailbox_destroy (&mbx);

  free (filename);
  msgset_free (msglist);
  return 0;
}

int
mail_copy (int argc, char **argv)
{
  return mail_copy0 (argc, argv, 0);
}
