/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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
 * w[rite] [file] -- GNU extension
 * w[rite] [msglist] file
 * W[rite] [msglist] -- GNU extension
 */

/*
 * NOTE: outfolder variable
 */

int
mail_write (int argc, char **argv)
{
  message_t msg;
  body_t bod;
  stream_t stream;
  FILE *output;
  char *filename = NULL;
  char buffer[512];
  off_t off = 0;
  size_t n = 0;
  int *msglist = NULL;
  int num = 0, i = 0;
  int sender = 0;

  if (isupper (argv[0][0]))
    sender = 1;
  else if (argc >= 2)
    filename = strdup (argv[--argc]);
  else
    filename = strdup ("mbox");

  num = util_expand_msglist (argc, argv, &msglist);

  if (sender)
    {
      filename = util_get_sender(msglist[0], 1);
      if (!filename)
	{
	  free (msglist);
	  return 1;
	}
    }

  output = fopen (filename, "a");

  for (i = 0; i < num; i++)
    {
      mailbox_get_message (mbox, msglist[i], &msg);
      message_get_body (msg, &bod);
      body_get_stream (bod, &stream);
      /* should there be a separator? */
      while (stream_read(stream, buffer, sizeof (buffer) - 1, off, &n)
	     == 0 && n != 0)
	{
	  buffer[n] = '\0';
	  fprintf (output, "%s", buffer);
	  off += n;
	}
      /* mark as saved */
    }

  fclose (output);
  free (msglist);
  return 0;
}
